#include "cell_transition_manager.h"
#include "world_manager.h"
#include <algorithm>
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

void CellTransitionManager::update(float deltaTime) {
    if (!worldManager) return;

    // WorldManager owns cell streaming (radius based, capped at MAX_ACTIVE_CELLS).
    // Mirror its active-cell set instead of loading/unloading cells here, so the two
    // systems cannot fight over the active-cell budget.
    syncFromActiveCells();

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

void CellTransitionManager::syncFromActiveCells() {
    if (!worldManager) return;

    for (auto& state : cellLoadStates) {
        state.isLoaded = false;
    }

    for (const auto& cell : worldManager->getActiveCells()) {
        if (!cell) continue;

        auto it = std::find_if(
            cellLoadStates.begin(), cellLoadStates.end(),
            [&cell](const CellLoadState& state) {
                return state.cellX == cell->cellX && state.cellY == cell->cellY;
            });

        if (it != cellLoadStates.end()) {
            it->isLoaded = true;
            continue;
        }

        CellLoadState newState;
        newState.cellX = cell->cellX;
        newState.cellY = cell->cellY;
        newState.isLoaded = true;
        newState.timeSinceLoaded = 0.0f;
        cellLoadStates.push_back(newState);
    }

    // Drop cells that are no longer active so the tracked set stays in sync.
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
