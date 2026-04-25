#include "cell_transition_manager.h"
#include "world_manager.h"
#include <cmath>

CellTransitionManager::CellTransitionManager()
    : worldManager(nullptr) {
    LOGD("CellTransitionManager created");
}

CellTransitionManager::~CellTransitionManager() {
    cleanup();
}

bool CellTransitionManager::initialize(WorldManager* worldMgr) {
    if (!worldMgr) {
        LOGE("Cannot initialize CellTransitionManager with null WorldManager");
        return false;
    }

    worldManager = worldMgr;
    LOGI("CellTransitionManager initialized");
    return true;
}

void CellTransitionManager::cleanup() {
    cellLoadStates.clear();
    worldManager = nullptr;
    LOGD("CellTransitionManager cleaned up");
}

void CellTransitionManager::update(const glm::vec3& playerPos, float deltaTime) {
    if (!worldManager) return;

    // Get current cell
    int32_t cellX, cellY;
    getCurrentCell(playerPos, cellX, cellY);

    // Load adjacent cells
    loadAdjacentCells(cellX, cellY);

    // Unload distant cells
    unloadDistantCells(cellX, cellY);

    // Update timeSinceLoaded for tracking
    for (auto& state : cellLoadStates) {
        if (state.isLoaded) {
            state.timeSinceLoaded += deltaTime;
        }
    }
}

void CellTransitionManager::getCurrentCell(const glm::vec3& pos,
                                           int32_t& outCellX, int32_t& outCellY) const {
    outCellX = static_cast<int32_t>(pos.x / CELL_SIZE);
    outCellY = static_cast<int32_t>(pos.z / CELL_SIZE);
}

void CellTransitionManager::getNearbyLoadedCells(const glm::vec3& pos,
                                                std::vector<std::pair<int32_t, int32_t>>& outCells) const {
    int32_t centerX, centerY;
    getCurrentCell(pos, centerX, centerY);

    outCells.clear();
    for (const auto& state : cellLoadStates) {
        if (state.isLoaded) {
            int32_t dx = std::abs(state.cellX - centerX);
            int32_t dy = std::abs(state.cellY - centerY);
            if (dx <= LOAD_DISTANCE && dy <= LOAD_DISTANCE) {
                outCells.push_back({state.cellX, state.cellY});
            }
        }
    }
}

bool CellTransitionManager::shouldLoadCell(int32_t cellX, int32_t cellY) const {
    // Find if cell exists in load states
    for (const auto& state : cellLoadStates) {
        if (state.cellX == cellX && state.cellY == cellY) {
            return !state.isLoaded;  // Should load if not already loaded
        }
    }
    return true;  // New cell, should load
}

bool CellTransitionManager::shouldUnloadCell(int32_t cellX, int32_t cellY) const {
    // Find if cell exists in load states
    for (const auto& state : cellLoadStates) {
        if (state.cellX == cellX && state.cellY == cellY) {
            return state.isLoaded;  // Should unload if loaded
        }
    }
    return false;  // Cell doesn't exist, nothing to unload
}

void CellTransitionManager::loadAdjacentCells(int32_t centerCellX, int32_t centerCellY) {
    if (!worldManager) return;

    // Load 5x5 grid around player
    for (int32_t dx = -LOAD_DISTANCE; dx <= LOAD_DISTANCE; ++dx) {
        for (int32_t dy = -LOAD_DISTANCE; dy <= LOAD_DISTANCE; ++dy) {
            int32_t cellX = centerCellX + dx;
            int32_t cellY = centerCellY + dy;

            // Check if cell already exists in load states
            bool cellExists = false;
            for (auto& state : cellLoadStates) {
                if (state.cellX == cellX && state.cellY == cellY) {
                    cellExists = true;
                    if (!state.isLoaded) {
                        // Load the cell
                        if (worldManager->loadCell(cellX, cellY)) {
                            state.isLoaded = true;
                            state.timeSinceLoaded = 0.0f;
                            LOGD("Cell (%d, %d) loaded", cellX, cellY);
                        }
                    }
                    break;
                }
            }

            // If cell doesn't exist, create new load state and load it
            if (!cellExists) {
                if (worldManager->loadCell(cellX, cellY)) {
                    CellLoadState newState;
                    newState.cellX = cellX;
                    newState.cellY = cellY;
                    newState.isLoaded = true;
                    newState.timeSinceLoaded = 0.0f;
                    cellLoadStates.push_back(newState);
                    LOGI("New cell (%d, %d) loaded", cellX, cellY);
                }
            }
        }
    }
}

void CellTransitionManager::unloadDistantCells(int32_t centerCellX, int32_t centerCellY) {
    if (!worldManager) return;

    // Unload cells beyond UNLOAD_DISTANCE
    for (auto it = cellLoadStates.begin(); it != cellLoadStates.end(); ++it) {
        if (!it->isLoaded) continue;

        int32_t dx = std::abs(it->cellX - centerCellX);
        int32_t dy = std::abs(it->cellY - centerCellY);

        if (dx > UNLOAD_DISTANCE || dy > UNLOAD_DISTANCE) {
            // Unload the cell
            worldManager->unloadCell(it->cellX, it->cellY);
            it->isLoaded = false;
            it->timeSinceLoaded = 0.0f;
            LOGD("Cell (%d, %d) unloaded (distance exceeded)", it->cellX, it->cellY);
        }
    }

    // Remove unloaded cells from tracking (optional - keeps memory clean)
    cellLoadStates.erase(
        std::remove_if(cellLoadStates.begin(), cellLoadStates.end(),
                      [](const CellLoadState& state) { return !state.isLoaded; }),
        cellLoadStates.end()
    );
}

void CellTransitionManager::transferNPCsBetweenCells(int32_t oldCellX, int32_t oldCellY,
                                                     int32_t newCellX, int32_t newCellY) {
    if (!worldManager) return;

    // Get NPCs in old cell and move them to new cell
    auto oldCell = worldManager->getCell(oldCellX, oldCellY);
    auto newCell = worldManager->getCell(newCellX, newCellY);

    if (oldCell && newCell) {
        // This would be handled by NpcManager for tracking which NPCs are in which cell
        LOGD("NPCs transferred from cell (%d, %d) to (%d, %d)",
             oldCellX, oldCellY, newCellX, newCellY);
    }
}
