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
    LOGD_DOOR("Door registered: ID=%u, Name=%s, Dest Cell=%u",
             door.doorId, door.name.c_str(), door.destinationCell);
}

void DoorManager::registerDoor(uint32_t doorId, const glm::vec3& position,
                              const std::string& nameEn, const std::string& nameJa,
                              uint32_t destinationCell, const glm::vec3& destinationPos) {
    Door door(doorId, position, nameEn, nameJa, destinationCell, destinationPos);
    registerDoor(door);
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

std::vector<const Door*> DoorManager::getDoorsInCell(uint32_t cellId) const {
    std::vector<const Door*> result;

    for (const auto& pair : doors) {
        const Door& door = pair.second;
        // A door in a cell is one that leads FROM that cell
        // This is implicit based on the cell it's located in
        // For now, we can't determine this without additional metadata
        // This method is for future enhancement
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

    // 2. Load destination cell
    if (!worldManager->loadCell(door.destinationCell)) {
        LOGE_DOOR("Failed to load destination cell: ID=%u", door.destinationCell);
        return false;
    }

    // 3. Set player position at destination
    worldManager->setPlayerPosition(door.destinationPos);

    // 4. Set player rotation if specified
    if (door.destinationRotation.x != 0.0f || door.destinationRotation.y != 0.0f || door.destinationRotation.z != 0.0f) {
        worldManager->setPlayerRotation(door.destinationRotation);
    }

    // 5. Update active cells (will unload distant cells, including old cell if needed)
    worldManager->updateActiveCells();

    LOGI_DOOR("Cell transition complete: %u → %u via door ID=%u",
             oldCellId, door.destinationCell, door.doorId);

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
        LOGD_DOOR("  Door: %s (ID=%u, Cell %u → %u)",
                 door.name.c_str(), door.doorId,
                 0, door.destinationCell);  // Note: 0 is placeholder for source cell
    }

    LOGD_DOOR("========================================");
}
