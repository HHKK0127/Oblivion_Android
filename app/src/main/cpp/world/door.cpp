#include "door.h"
#include "world_manager.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// DoorManager Implementation
// ============================================================================

DoorManager::DoorManager()
    : worldManager(nullptr), nextDoorId(5000) {
    LOGD_DOOR("DoorManager created");
}

DoorManager::~DoorManager() {
    cleanup();
    LOGD_DOOR("DoorManager destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

bool DoorManager::initialize(WorldManager* worldMgr) {
    if (!worldMgr) {
        LOGE_DOOR("Cannot initialize DoorManager with null WorldManager");
        return false;
    }

    worldManager = worldMgr;
    LOGI_DOOR("DoorManager initialized with WorldManager");
    return true;
}

void DoorManager::cleanup() {
    doors.clear();
    worldManager = nullptr;
    LOGD_DOOR("DoorManager cleaned up");
}

// ============================================================================
// Door Registration
// ============================================================================

void DoorManager::registerDoor(const Door& door) {
    if (!validateDoorData(door)) {
        LOGW_DOOR("Attempted to register invalid door data");
        return;
    }

    doors[door.doorId] = door;
    // The ESM corpus holds ~4,400 door references and registerEsmDoors() walks
    // all of them in one go, so a per-door DEBUG line would flood logcat.
    // DoorManager::logDoorStatus() dumps the full table on demand instead.
    if (doors.size() <= 4) {
        LOGD_DOOR("Door registered: ID=%u, Name=%s, Cell 0x%08X -> 0x%08X",
                 door.doorId, door.name.c_str(), door.sourceCellFormID,
                 door.destinationCell);
    } else if (doors.size() == 5) {
        LOGD_DOOR("Door registration log throttled (use logDoorStatus for the full table)");
    }
}

void DoorManager::registerDoor(uint32_t doorId, const glm::vec3& position,
                              const std::string& nameEn, const std::string& nameJa,
                              uint32_t destinationCell, const glm::vec3& destinationPos) {
    Door door(doorId, position, nameEn, nameJa, destinationCell, destinationPos);
    registerDoor(door);
}

void DoorManager::clearDoors() {
    if (!doors.empty()) {
        LOGI_DOOR("Clearing %zu registered doors", doors.size());
    }
    doors.clear();
}

// ============================================================================
// Door Queries
// ============================================================================

const Door* DoorManager::getDoor(uint32_t doorId) const {
    auto it = doors.find(doorId);
    if (it == doors.end()) {
        return nullptr;
    }
    return &it->second;
}

const Door* DoorManager::getDoorAtPosition(const glm::vec3& position, float radius) const {
    float closestDistance = radius;
    const Door* closestDoor = nullptr;

    for (const auto& pair : doors) {
        const Door& door = pair.second;
        glm::vec3 diff = door.position - position;
        float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

        if (distance < closestDistance) {
            closestDistance = distance;
            closestDoor = &door;
        }
    }

    return closestDoor;
}

const Door* DoorManager::getNearestDoor(const glm::vec3& position) const {
    if (doors.empty()) {
        return nullptr;
    }

    float closestDistance = FLT_MAX;
    const Door* closestDoor = nullptr;

    for (const auto& pair : doors) {
        const Door& door = pair.second;
        glm::vec3 diff = door.position - position;
        float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

        if (distance < closestDistance) {
            closestDistance = distance;
            closestDoor = &door;
        }
    }

    return closestDoor;
}

std::vector<const Door*> DoorManager::getDoorsInCell(uint32_t cellFormID) const {
    std::vector<const Door*> result;

    if (cellFormID == 0) {
        return result;
    }

    for (const auto& pair : doors) {
        const Door& door = pair.second;
        if (door.sourceCellFormID == cellFormID) {
            result.push_back(&door);
        }
    }

    return result;
}

// ============================================================================
// Door Interaction
// ============================================================================

bool DoorManager::useDoor(uint32_t doorId) {
    const Door* door = getDoor(doorId);
    if (!door) {
        LOGW_DOOR("Attempted to use non-existent door: ID=%u", doorId);
        return false;
    }

    LOGI_DOOR("Using door: ID=%u, Name=%s", doorId, door->name.c_str());

    return performCellTransition(*door);
}

bool DoorManager::performCellTransition(const Door& door) {
    if (!worldManager) {
        LOGE_DOOR("WorldManager not initialized, cannot perform cell transition");
        return false;
    }

    // ========================================================================
    // Cell Transition Logic (Task 2)
    // ========================================================================

    // 1. Get current cell before transition
    auto currentCell = worldManager->getCurrentCell();
    uint32_t oldCellId = currentCell ? currentCell->cellId : 0;
    const bool wasIndoors = worldManager->isPlayerIndoors();

    // 2. Move the player to the destination transform *before* switching cells.
    // WorldManager records the position even while the player is indoors, and
    // leaveInteriorCell() resolves the resumed exterior cell from it, so the
    // destination has to be in place first or the player resumes streaming at
    // the coordinate they left.
    worldManager->setPlayerPosition(door.destinationPos);
    if (door.destinationRotation.x != 0.0f || door.destinationRotation.y != 0.0f ||
        door.destinationRotation.z != 0.0f) {
        worldManager->setPlayerRotation(door.destinationRotation);
    }

    // 3. Switch cells. Interior cells are keyed by TES FormID and carry no grid
    // coordinate, so they go through enterInteriorCell(); an exterior
    // destination is a registered cell and is reached by its grid square.
    if (door.destinationIsInterior) {
        // enterInteriorCell() replaces currentCell outright, so a preceding
        // leaveInteriorCell() would only add a pointless exterior re-resolve.
        if (!worldManager->enterInteriorCell(door.destinationCell,
                                            door.destinationEditorID,
                                            door.destinationCellName)) {
            LOGE_DOOR("Failed to enter interior cell: 0x%08X", door.destinationCell);
            return false;
        }
    } else {
        auto destCell = worldManager->getCellByFormID(door.destinationCell);
        if (!destCell) {
            LOGE_DOOR("Destination cell 0x%08X is not loaded in this worldspace",
                      door.destinationCell);
            return false;
        }
        // Drop the interior first: while the player is marked indoors,
        // getCellAt() is bypassed and the exterior never becomes current.
        if (wasIndoors) {
            worldManager->leaveInteriorCell();
        }
        if (!worldManager->loadCell(destCell->cellX, destCell->cellY)) {
            LOGE_DOOR("Failed to load destination cell: grid (%d, %d)",
                      destCell->cellX, destCell->cellY);
            return false;
        }
    }

    // 4. Stream cells around the destination. Skipped indoors: interior
    // positions are cell relative, so streaming would pull the exterior cells
    // sharing that coordinate back in.
    if (!worldManager->isPlayerIndoors()) {
        worldManager->updateActiveCells();
    }

    auto landed = worldManager->getCurrentCell();
    LOGI_DOOR("Cell transition complete: %u -> %u via door ID=%u (0x%08X -> 0x%08X, %s)",
             oldCellId, landed ? landed->cellId : 0,
             door.doorId, door.sourceCellFormID, door.destinationCell,
             door.destinationIsInterior ? "interior" : "exterior");

    return true;
}

// ============================================================================
// Validation & Status
// ============================================================================

bool DoorManager::validateDoorData(const Door& door) const {
    if (door.doorId == 0) {
        LOGW_DOOR("Door ID cannot be 0");
        return false;
    }

    if (door.destinationCell == 0) {
        LOGW_DOOR("Destination cell ID cannot be 0");
        return false;
    }

    if (door.name.empty()) {
        LOGW_DOOR("Door name cannot be empty");
        return false;
    }

    if (door.interactionRadius <= 0.0f) {
        LOGW_DOOR("Door interaction radius must be positive");
        return false;
    }

    return true;
}

void DoorManager::logDoorStatus() const {
    LOGD_DOOR("========== Door Manager Status ==========");
    LOGD_DOOR("Total doors: %zu", doors.size());

    for (const auto& pair : doors) {
        const Door& door = pair.second;
        LOGD_DOOR("  Door: %s (ID=%u, Cell 0x%08X -> 0x%08X%s)",
                 door.name.c_str(), door.doorId,
                 door.sourceCellFormID, door.destinationCell,
                 door.destinationIsInterior ? " interior" : "");
    }

    LOGD_DOOR("========================================");
}
