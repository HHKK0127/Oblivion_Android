#include "world_manager.h"
#include "cell_loader.h"
#include "cell_transition_manager.h"
#include "door.h"
#include "../game/npc_manager.h"
#include "../assets/asset_manager.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// WorldManager Implementation
// ============================================================================

WorldManager::WorldManager()
    : npcManager(nullptr), assetManager(nullptr),
      cellLoadRadius(DEFAULT_CELL_LOAD_RADIUS),
      cellUnloadRadius(DEFAULT_CELL_UNLOAD_RADIUS),
      loadUpdateTimer(0.5f), nextCellId(1), nextWorldItemId(1000),
      cellsLoaded(0), cellsUnloaded(0), currentWorldspaceFormID(0) {
}

WorldManager::~WorldManager() {
    cleanup();
}

// ============================================================================
// Lifecycle Methods
// ============================================================================

bool WorldManager::initialize(NpcManager* npcMgr, AssetManager* assetMgr) {
    npcManager = npcMgr;
    assetManager = assetMgr;

    LOGI_WORLD("WorldManager initializing...");

    // Initialize cell loader
    if (!cellManager.initialize(npcMgr, assetMgr)) {
        LOGE_WORLD("Failed to initialize CellManager");
        return false;
    }

    // Initialize CellTransitionManager (Phase 3+)
    cellTransitionManager = std::make_unique<CellTransitionManager>();
    if (!cellTransitionManager->initialize(this)) {
        LOGE_WORLD("Failed to initialize CellTransitionManager");
        return false;
    }
    LOGI_WORLD("CellTransitionManager initialized");

    // Initialize DoorManager (Phase 4 Task 2)
    doorManager = std::make_unique<DoorManager>();
    if (!doorManager->initialize(this)) {
        LOGE_WORLD("Failed to initialize DoorManager");
        return false;
    }
    LOGI_WORLD("DoorManager initialized");

    // Create test cells
    initializeTestCells();

    LOGI_WORLD("WorldManager initialized: %zu cells created", cells.size());
    return true;
}

void WorldManager::update(float deltaTime) {
    loadUpdateTimer += deltaTime;

    // Update active cells
    for (auto& cell : activeCells) {
        if (cell && cell->isLoaded()) {
            cellManager.updateCell(cell, deltaTime);
        }
    }

    // Update cell transitions (Phase 3+)
    if (cellTransitionManager) {
        cellTransitionManager->update(deltaTime);
    }

    // Batch cell loading/unloading every 0.5 seconds
    if (loadUpdateTimer >= 0.5f) {
        updateActiveCells();
        loadUpdateTimer = 0.0f;
    }

    // Advance world time
    advanceTime(deltaTime);
}

void WorldManager::render() {
    // Render all active cells
    for (auto& cell : activeCells) {
        if (cell && cell->isLoaded()) {
            cellManager.renderCell(cell);
        }
    }
}

void WorldManager::cleanup() {
    LOGI_WORLD("WorldManager cleaning up...");

    // Cleanup CellTransitionManager (Phase 3+)
    if (cellTransitionManager) {
        cellTransitionManager->cleanup();
        cellTransitionManager = nullptr;
    }

    // Cleanup DoorManager (Phase 4 Task 2)
    if (doorManager) {
        doorManager->cleanup();
        doorManager = nullptr;
    }

    // Unload all cells
    for (auto& cell : activeCells) {
        if (cell) {
            cellManager.unloadCell(cell);
        }
    }

    activeCells.clear();
    cells.clear();
    coordToId.clear();
    currentCell = nullptr;

    LOGI_WORLD("WorldManager cleanup complete");
}

// ============================================================================
// Cell Management
// ============================================================================

bool WorldManager::loadCell(uint32_t cellId) {
    auto it = cells.find(cellId);
    if (it == cells.end()) {
        LOGW_WORLD("Cell ID %u not found", cellId);
        return false;
    }

    return loadCell(it->second->cellX, it->second->cellY);
}

bool WorldManager::loadCell(int32_t cellX, int32_t cellY) {
    uint32_t cellId = getOrCreateCellId(cellX, cellY);
    if (cellId == 0) {
        return false;
    }

    auto it = cells.find(cellId);
    if (it == cells.end()) {
        return false;
    }

    auto cell = it->second;
    if (cell->isLoaded()) {
        return true;  // Already loaded
    }

    // Load cell data
    if (!cellManager.loadCell(cell)) {
        LOGE_WORLD("Failed to load cell %u (%d, %d)", cellId, cellX, cellY);
        return false;
    }

    // ========================================================================
    // NPC Integration (Task 1-4): Load NPCs for this cell
    // ========================================================================
    if (npcManager) {
        // Get NPCs that should be in this cell (from cell loader or stored data)
        // For now, query any NPCs registered to this cell by the system
        auto cellNpcs = npcManager->getNpcsForCell(cellId);
        LOGD_WORLD("Cell %u (%d, %d) loaded with %zu NPCs", cellId, cellX, cellY, cellNpcs.size());

        // NPCs are already registered via registerNpcToCell() during world initialization
        // or cell loading. This is just logging for verification.
    }

    // Add to active cells if not already there
    auto activeIt = std::find(activeCells.begin(), activeCells.end(), cell);
    if (activeIt == activeCells.end() && activeCells.size() < MAX_ACTIVE_CELLS) {
        activeCells.push_back(cell);
    }

    cellsLoaded++;
    return true;
}

void WorldManager::unloadCell(uint32_t cellId) {
    auto it = cells.find(cellId);
    if (it == cells.end()) {
        return;
    }

    auto cell = it->second;

    // ========================================================================
    // NPC Integration (Task 1-4): Unload NPCs from this cell
    // ========================================================================
    if (npcManager) {
        // Get NPCs currently in this cell
        auto cellNpcs = npcManager->getNpcsForCell(cellId);

        // Unregister each NPC from the cell
        for (auto& npc : cellNpcs) {
            if (npc) {
                npcManager->unregisterNpcFromCell(npc->npcId);
                LOGD_WORLD("NPC %u unregistered from cell %u", npc->npcId, cellId);
            }
        }

        LOGD_WORLD("Cell %u unloading: %zu NPCs unregistered", cellId, cellNpcs.size());
    }

    cellManager.unloadCell(cell);

    // Remove from active cells
    auto activeIt = std::find(activeCells.begin(), activeCells.end(), cell);
    if (activeIt != activeCells.end()) {
        activeCells.erase(activeIt);
    }

    cellsUnloaded++;
}

void WorldManager::unloadCell(int32_t cellX, int32_t cellY) {
    uint32_t cellId = getOrCreateCellId(cellX, cellY);
    unloadCell(cellId);
}

void WorldManager::updateActiveCells() {
    checkCellDistances();
    unloadDistantCells();
    loadNearbyCells();

    logWorldStatus();
}

// ============================================================================
// Player Position Tracking
// ============================================================================

void WorldManager::setPlayerPosition(const glm::vec3& pos) {
    worldState.playerPosition = pos;

    // Update current cell
    auto newCell = getCellAt(pos);
    if (newCell && newCell != currentCell) {
        currentCell = newCell;
        LOGD_WORLD("Player entered cell: %s", currentCell->cellName.c_str());
    }
}

void WorldManager::setPlayerRotation(const glm::vec3& rot) {
    worldState.playerRotation = rot;
}

// ============================================================================
// Cell Queries
// ============================================================================

std::shared_ptr<Cell> WorldManager::getCellAt(const glm::vec3& worldPos) {
    CellCoord coord = CellCoordUtils::getCoordFromWorldPos(worldPos);
    return getCellByCoord(coord.x, coord.y);
}

std::shared_ptr<Cell> WorldManager::getCellById(uint32_t cellId) {
    auto it = cells.find(cellId);
    if (it != cells.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Cell> WorldManager::getCellByCoord(int32_t cellX, int32_t cellY) {
    // Lookup only: cells are created by createCell() / addCellFromESM(). Creating
    // them on demand would fabricate empty flat cells for any queried coordinate.
    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) | static_cast<uint32_t>(cellY);
    auto it = coordToId.find(key);
    if (it == coordToId.end()) {
        return nullptr;
    }
    return getCellById(it->second);
}

std::vector<NPC*> WorldManager::getNpcsInCell(uint32_t cellId) {
    auto cell = getCellById(cellId);
    if (!cell) return {};
    return cellManager.getNpcsInCell(cell);
}

std::vector<NPC*> WorldManager::getNpcsInCell(std::shared_ptr<Cell> cell) {
    if (!cell) return {};
    return cellManager.getNpcsInCell(cell);
}

// ============================================================================
// World State
// ============================================================================

void WorldManager::advanceTime(float deltaTime) {
    worldState.timeOfDay += deltaTime * 0.1f;  // 1 game second = 0.1 real seconds (approx)

    if (worldState.timeOfDay >= 24.0f) {
        worldState.timeOfDay -= 24.0f;
        worldState.dayCount++;
    }
}

// ============================================================================
// Memory Management
// ============================================================================

size_t WorldManager::getTotalMemoryUsage() const {
    size_t total = 0;
    for (const auto& pair : cells) {
        if (pair.second) {
            total += pair.second->memoryUsage;
        }
    }
    return total;
}

void WorldManager::logWorldStatus() const {
    size_t totalMemory = getTotalMemoryUsage();
    LOGD_WORLD("World Status: %zu cells, %zu active, Memory: %.2f MB, Day: %u, Time: %.1f",
               cells.size(), activeCells.size(),
               static_cast<float>(totalMemory) / (1024.0f * 1024.0f),
               worldState.dayCount, worldState.timeOfDay);
}

// ============================================================================
// Private Methods
// ============================================================================

void WorldManager::initializeTestCells() {
    // Create test cells - 3x3 grid
    for (int32_t y = -1; y <= 1; y++) {
        for (int32_t x = -1; x <= 1; x++) {
            std::string name = "TestCell_" + std::to_string(x) + "_" + std::to_string(y);
            createCell(nextCellId++, x, y, name, CellType::EXTERIOR);
        }
    }

    // Load center cell (0, 0) as starting point
    loadCell(0, 0);
    currentCell = getCellByCoord(0, 0);

    if (currentCell) {
        setPlayerPosition(glm::vec3(64.0f, 64.0f, 0.0f));
    }

    // ========================================================================
    // Register Test Doors (Task 2)
    // ========================================================================
    if (doorManager) {
        // Door from center cell (0, 0) to right cell (1, 0)
        doorManager->registerDoor(1000, glm::vec3(128.0f, 64.0f, 0.0f),
                                 "Door to East", "Door to East",
                                 2, glm::vec3(0.0f, 64.0f, 0.0f));

        // Door from center cell (0, 0) to top cell (0, 1)
        doorManager->registerDoor(1001, glm::vec3(64.0f, 128.0f, 0.0f),
                                 "Door to North", "Door to North",
                                 5, glm::vec3(64.0f, 0.0f, 0.0f));

        // Door from center cell (0, 0) to bottom cell (0, -1)
        doorManager->registerDoor(1002, glm::vec3(64.0f, 0.0f, 0.0f),
                                 "Door to South", "Door to South",
                                 8, glm::vec3(64.0f, 128.0f, 0.0f));

        LOGI_WORLD("Test doors registered: %zu doors", doorManager->getDoorCount());
    }

    LOGI_WORLD("Test cells initialized: %zu cells", cells.size());
}

std::shared_ptr<Cell> WorldManager::createCell(uint32_t cellId, int32_t cellX, int32_t cellY,
                                              const std::string& name, CellType type) {
    auto cell = std::make_shared<Cell>();
    cell->cellId = cellId;
    cell->cellX = cellX;
    cell->cellY = cellY;
    cell->cellName = name;
    cell->cellType = type;
    cell->loadState = CellLoadState::UNLOADED;

    cells[cellId] = cell;
    coordToId[(static_cast<uint64_t>(cellX) << 32) | (cellY & 0xFFFFFFFF)] = cellId;

    // Cell creation is reported in aggregate by the world builder; logging each
    // one here produces ~14k lines per start-up.
    (void)name;
    return cell;
}

void WorldManager::checkCellDistances() {
    // distanceFromPlayer is only consumed by shouldLoadCell() and shouldUnloadCell(),
    // which are evaluated for the currently active cells and for the 3x3 block around
    // the player. Refreshing all ~14k registered cells every frame was pure overhead.
    const CellCoord center = currentCell
        ? CellCoord(currentCell->cellX, currentCell->cellY)
        : CellCoordUtils::getCoordFromWorldPos(worldState.playerPosition);

    const float halfCell = CELL_SIZE * 0.5f;

    for (const auto& cell : activeCells) {
        updateCellDistance(cell, halfCell);
    }

    for (const auto& coord : CellCoordUtils::getAdjacentCoords(center)) {
        updateCellDistance(getCellByCoord(coord.x, coord.y), halfCell);
    }
}

void WorldManager::updateCellDistance(const std::shared_ptr<Cell>& cell, float halfCell) {
    if (!cell) return;

    // The world is Y-up: the distance that decides cell streaming is horizontal, from
    // the player to the cell CENTRE. Measuring to the cell corner and including the
    // height component made every neighbouring cell look out of range, so only the
    // spawn cell was ever loaded.
    const float cellCenterX = static_cast<float>(cell->cellX) * CELL_SIZE + halfCell;
    const float cellCenterZ = static_cast<float>(cell->cellY) * CELL_SIZE + halfCell;

    const float dx = worldState.playerPosition.x - cellCenterX;
    const float dz = worldState.playerPosition.z - cellCenterZ;
    cell->distanceFromPlayer = dx * dx + dz * dz;
}

void WorldManager::unloadDistantCells() {
    std::vector<uint32_t> toUnload;

    for (auto& cell : activeCells) {
        if (cell && shouldUnloadCell(cell)) {
            toUnload.push_back(cell->cellId);
        }
    }

    for (uint32_t cellId : toUnload) {
        unloadCell(cellId);
    }
}

void WorldManager::loadNearbyCells() {
    // Get all adjacent cells from current cell
    if (!currentCell) return;

    std::vector<CellCoord> toLoad = CellCoordUtils::getAdjacentCoords(
        CellCoord(currentCell->cellX, currentCell->cellY)
    );

    const float loadRadiusSq = cellLoadRadius * cellLoadRadius;

    for (const auto& coord : toLoad) {
        auto cell = getCellByCoord(coord.x, coord.y);
        if (!cell) continue;
        if (cell->distanceFromPlayer > loadRadiusSq) continue;

        if (cell->isLoaded()) {
            // A cell can be loaded without being in activeCells (door transitions, saves).
            // Re-activate it so it is updated and rendered again.
            if (activeCells.size() < MAX_ACTIVE_CELLS &&
                std::find(activeCells.begin(), activeCells.end(), cell) == activeCells.end()) {
                activeCells.push_back(cell);
            }
            continue;
        }

        loadCell(coord.x, coord.y);
    }
}

bool WorldManager::shouldLoadCell(std::shared_ptr<Cell> cell) const {
    if (!cell) return false;
    if (cell->isLoaded()) return false;

    // Load cells within load radius (using squared distance comparison)
    return cell->distanceFromPlayer <= cellLoadRadius * cellLoadRadius;
}

bool WorldManager::shouldUnloadCell(std::shared_ptr<Cell> cell) const {
    if (!cell) return false;
    if (!cell->isLoaded()) return false;

    // Unload cells beyond unload radius (using squared distance comparison)
    return cell->distanceFromPlayer > cellUnloadRadius * cellUnloadRadius;
}

uint32_t WorldManager::getOrCreateCellId(int32_t cellX, int32_t cellY) {
    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) | static_cast<uint32_t>(cellY);
    auto it = coordToId.find(key);
    if (it != coordToId.end()) {
        return it->second;
    }
    return 0;
}

std::shared_ptr<Cell> WorldManager::getCellByFormID(uint32_t tesFormID) const {
    if (tesFormID == 0) return nullptr;

    auto lookup = formIdToCellId.find(tesFormID);
    if (lookup != formIdToCellId.end()) {
        auto it = cells.find(lookup->second);
        if (it != cells.end()) {
            return it->second;
        }
    }
    return nullptr;
}

std::shared_ptr<Cell> WorldManager::addCellFromESM(int32_t cellX, int32_t cellY,
                                                     const std::string& editorID,
                                                     const std::string& fullName,
                                                     uint32_t tesFormID,
                                                     bool isExterior,
                                                     uint32_t worldspaceFormID) {
    // Interior cells have no grid position. Giving them one would make them
    // collide with the real exterior cell that owns that coordinate, so they
    // are skipped entirely.
    if (!isExterior) {
        return nullptr;
    }

    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) | static_cast<uint32_t>(cellY);
    auto it = coordToId.find(key);
    if (it != coordToId.end()) {
        return cells[it->second];
    }

    uint32_t cellId = nextCellId++;
    std::string name = fullName.empty() ? editorID : fullName;
    if (name.empty()) {
        name = "Cell_" + std::to_string(cellX) + "_" + std::to_string(cellY);
    }

    auto cell = createCell(cellId, cellX, cellY, name, CellType::EXTERIOR);
    cell->editorID = editorID;
    cell->tesFormID = tesFormID;
    cell->worldspaceFormID = worldspaceFormID;
    if (tesFormID != 0) {
        formIdToCellId[tesFormID] = cellId;
    }
    return cell;
}

std::shared_ptr<Cell> WorldManager::enterInteriorCell(uint32_t tesFormID,
                                                      const std::string& editorID,
                                                      const std::string& fullName) {
    if (tesFormID == 0) return nullptr;

    // Re-entering the same interior reuses the existing cell.
    if (auto existing = getCellByFormID(tesFormID)) {
        currentCell = existing;
        return existing;
    }

    uint32_t cellId = nextCellId++;
    std::string name = fullName.empty() ? editorID : fullName;
    if (name.empty()) name = "Interior_" + std::to_string(tesFormID);

    // Interior cells carry no grid coordinate. createCell() would register a
    // (0,0) coordinate for them, which would collide with the real exterior
    // cell at that square, so build the cell directly and key it by FormID.
    auto cell = std::make_shared<Cell>();
    cell->cellId = cellId;
    cell->cellX = 0;
    cell->cellY = 0;
    cell->cellName = name;
    cell->editorID = editorID;
    cell->tesFormID = tesFormID;
    cell->worldspaceFormID = 0;
    cell->cellType = CellType::INTERIOR;
    cell->loadState = CellLoadState::LOADED;

    cells[cellId] = cell;
    formIdToCellId[tesFormID] = cellId;
    currentCell = cell;

    LOGI_WORLD("Entered interior cell %u (0x%08X) '%s'", cellId, tesFormID,
               name.c_str());
    return cell;
}

void WorldManager::clearAllCells() {
    activeCells.clear();
    currentCell.reset();
    cells.clear();
    coordToId.clear();
    formIdToCellId.clear();
    nextCellId = 1;
    cellsLoaded = 0;
    cellsUnloaded = 0;
}

size_t WorldManager::countExteriorCellsWithTerrain() const {
    size_t count = 0;
    for (const auto& pair : cells) {
        const auto& cell = pair.second;
        // A cell counts as having terrain when the LAND record was linked to it;
        // the dense grids are materialised on demand and must not gate this count.
        if (cell && cell->cellType == CellType::EXTERIOR &&
            (cell->hasTerrain || cell->hasDenseTerrain())) {
            ++count;
        }
    }
    return count;
}

bool WorldManager::spawnPlayerAtNearestTerrainCell() {
    std::shared_ptr<Cell> best;
    int64_t bestRank = 0;

    for (const auto& pair : cells) {
        const auto& cell = pair.second;
        if (!cell || cell->cellType != CellType::EXTERIOR) continue;
        if (!cell->hasTerrain && !cell->hasDenseTerrain()) continue;
        // Only consider cells of the worldspace the player belongs to.
        if (currentWorldspaceFormID != 0 && cell->worldspaceFormID != currentWorldspaceFormID) continue;

        const int64_t rank = static_cast<int64_t>(std::llabs(static_cast<long long>(cell->cellX))) +
                             static_cast<int64_t>(std::llabs(static_cast<long long>(cell->cellY)));
        if (!best || rank < bestRank) {
            best = cell;
            bestRank = rank;
        }
    }

    if (!best) {
        LOGW_WORLD("No exterior cell with terrain available for spawn");
        return false;
    }

    const float half = CELL_SIZE * 0.5f;
    float ground = 0.0f;
    if (best->hasDenseTerrain()) {
        ground = best->getTerrainHeightAt(half, half);
    } else if (assetManager) {
        // Dense grids are materialised on demand, so sample the compact LAND
        // record directly rather than expanding a whole cell just to spawn.
        for (const auto& terrain : assetManager->getEsmManager().getAllTerrains()) {
            if (terrain.formID == best->tesFormID && terrain.hasHeights()) {
                ground = terrain.heightAt(TERRAIN_RESOLUTION / 2, TERRAIN_RESOLUTION / 2);
                break;
            }
        }
    }

    loadCell(best->cellId);
    currentCell = best;

    // The heightmap is already expressed in world units, so the player only
    // needs a small clearance above the sampled surface.
    setPlayerPosition(glm::vec3(
        static_cast<float>(best->cellX) * CELL_SIZE + half,
        ground + 64.0f,
        static_cast<float>(best->cellY) * CELL_SIZE + half));

    LOGI_WORLD("Player spawned in cell %u (0x%08X) grid=(%d,%d) ground=%.1f",
               best->cellId, best->tesFormID, best->cellX, best->cellY, ground);
    return true;
}

CellCoord WorldManager::getPlayerCellCoord() const {
    return CellCoordUtils::getCoordFromWorldPos(worldState.playerPosition);
}

// ============================================================================
// World Items (Pickupable Objects)
// ============================================================================

uint32_t WorldManager::spawnWorldItem(uint32_t itemId, const std::string& itemName,
                                     const std::string& itemNameJa, const glm::vec3& position) {
    uint32_t worldItemId = nextWorldItemId++;
    auto worldItem = std::make_shared<WorldItem>(worldItemId, itemId, itemName, itemNameJa, position);
    worldItems.push_back(worldItem);

    LOGD_WORLD("Spawned world item: ID=%u, Name=%s at (%.1f, %.1f, %.1f)",
               worldItemId, itemName.c_str(), position.x, position.y, position.z);
    return worldItemId;
}

std::shared_ptr<WorldItem> WorldManager::getNearbyPickupItem(const glm::vec3& playerPos,
                                                              float pickupRange) const {
    for (const auto& item : worldItems) {
        if (item && item->isInPickupRange(playerPos, pickupRange)) {
            return item;
        }
    }
    return nullptr;
}

std::shared_ptr<WorldItem> WorldManager::getNearestWorldItem(const glm::vec3& playerPos) const {
    std::shared_ptr<WorldItem> nearest = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (const auto& item : worldItems) {
        if (!item || item->isPickedUp) continue;

        float distance = item->getDistanceTo(playerPos);
        if (distance < minDistance) {
            minDistance = distance;
            nearest = item;
        }
    }

    return nearest;
}

bool WorldManager::pickupWorldItem(uint32_t worldItemId) {
    for (auto& item : worldItems) {
        if (item && item->worldItemId == worldItemId) {
            item->isPickedUp = true;
            LOGD_WORLD("Picked up world item: ID=%u, Name=%s", worldItemId, item->itemName.c_str());
            return true;
        }
    }
    return false;
}
