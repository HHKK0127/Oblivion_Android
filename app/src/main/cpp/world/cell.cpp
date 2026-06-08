#include "cell.h"
#include "../game/npc.h"
#include "../geometry/mesh.h"
#include "../engine/shader.h"
#include "terrain.h"
#include <GLES3/gl3.h>

// ============================================================================
// CellManager Implementation
// ============================================================================

CellManager::CellManager() {
}

CellManager::~CellManager() {
}

bool CellManager::initialize(void* npcMgr, void* assetMgr) {
    // Initialize cell manager
    LOGD_CELL("CellManager initialized");
    return true;
}

bool CellManager::loadCell(std::shared_ptr<Cell> cell) {
    if (!cell) {
        LOGE_CELL("Cannot load null cell");
        return false;
    }

    if (cell->isLoaded()) {
        LOGW_CELL("Cell %u already loaded", cell->cellId);
        return true;
    }

    LOGI_CELL("Loading cell: %s (ID: %u, Coord: %d,%d)",
              cell->cellName.c_str(), cell->cellId, cell->cellX, cell->cellY);

    // Mark as loading
    cell->loadState = CellLoadState::LOADING;

    // Load terrain
    if (!createTerrainMesh(cell)) {
        LOGE_CELL("Failed to create terrain mesh for cell %u", cell->cellId);
        cell->loadState = CellLoadState::UNLOADED;
        return false;
    }

    // Load NPCs (to be implemented with NpcManager)
    // loadNpcsForCell(cell);

    // Load objects
    // loadObjectsForCell(cell);

    // Mark as loaded
    cell->loadState = CellLoadState::LOADED;
    cell->isDirty = false;

    // Calculate memory usage
    cell->memoryUsage = calculateCellMemoryUsage(cell);

    LOGI_CELL("Cell loaded successfully: %s (Memory: %.2f MB)",
              cell->cellName.c_str(), cell->getMemoryUsageMB());

    return true;
}

void CellManager::unloadCell(std::shared_ptr<Cell> cell) {
    if (!cell || !cell->isLoaded()) {
        return;
    }

    LOGI_CELL("Unloading cell: %s (ID: %u)", cell->cellName.c_str(), cell->cellId);

    cell->loadState = CellLoadState::UNLOADING;

    // Unload terrain
    destroyTerrainMesh(cell);

    // Clear NPCs
    cell->npcs.clear();

    // Clear objects
    cell->staticObjects.clear();
    cell->dynamicObjects.clear();

    cell->loadState = CellLoadState::UNLOADED;
    cell->memoryUsage = 0;

    LOGD_CELL("Cell unloaded: %s", cell->cellName.c_str());
}

void CellManager::updateCell(std::shared_ptr<Cell> cell, float deltaTime) {
    if (!cell || !cell->isLoaded()) {
        return;
    }

    // Update NPCs in cell
    for (auto& npc : cell->npcs) {
        if (npc) {
            npc->update(deltaTime);
        }
    }

    // Update dynamic objects
    for (auto& obj : cell->dynamicObjects) {
        if (obj) {
            // Update object state if needed
        }
    }

    cell->isDirty = false;
}

void CellManager::renderCell(std::shared_ptr<Cell> cell) {
    if (!cell || !cell->isLoaded()) {
        return;
    }

    // TODO: ジオメトリレンダリングの実装
    // - 地形メッシュのレンダリング
    // - 静的オブジェクトのレンダリング
    // - 動的オブジェクトのレンダリング
    // - カメラ設定とシェーダーバインディング
}

bool CellManager::isCellLoaded(uint32_t cellId) const {
    // Would check in map (implemented in WorldManager)
    return false;
}

std::vector<NPC*> CellManager::getNpcsInCell(std::shared_ptr<Cell> cell) {
    std::vector<NPC*> result;
    if (!cell) return result;

    for (auto& npc : cell->npcs) {
        if (npc) {
            result.push_back(npc.get());
        }
    }
    return result;
}

std::vector<WorldObject*> CellManager::getObjectsInCell(std::shared_ptr<Cell> cell) {
    std::vector<WorldObject*> result;
    if (!cell) return result;

    for (auto& obj : cell->staticObjects) {
        if (obj) {
            result.push_back(obj.get());
        }
    }

    for (auto& obj : cell->dynamicObjects) {
        if (obj) {
            result.push_back(obj.get());
        }
    }
    return result;
}

size_t CellManager::calculateCellMemoryUsage(std::shared_ptr<Cell> cell) {
    if (!cell) return 0;

    size_t usage = 0;

    // Base cell structure
    usage += sizeof(Cell);

    // NPCs
    usage += cell->npcs.size() * sizeof(std::shared_ptr<NPC>);
    for (auto& npc : cell->npcs) {
        if (npc) {
            usage += sizeof(NPC);  // Simplified
        }
    }

    // Objects
    usage += cell->staticObjects.size() * sizeof(std::shared_ptr<WorldObject>);
    usage += cell->dynamicObjects.size() * sizeof(std::shared_ptr<WorldObject>);

    // Height data
    usage += cell->heightData.size() * sizeof(float);

    // Terrain mesh (estimate)
    usage += 1024 * 1024;  // ~1 MB estimate for terrain mesh

    return usage;
}

void CellManager::compactCellMemory(std::shared_ptr<Cell> cell) {
    if (!cell) return;

    // Compact height data
    cell->heightData.shrink_to_fit();

    // Compact containers
    std::vector<std::shared_ptr<NPC>>(cell->npcs).swap(cell->npcs);
    std::vector<std::shared_ptr<WorldObject>>(cell->staticObjects).swap(cell->staticObjects);
}

// ============================================================================
// Private Methods
// ============================================================================

bool CellManager::createTerrainMesh(std::shared_ptr<Cell> cell) {
    if (!cell) return false;

    // For now, create flat terrain
    cell->heightData = std::vector<float>(TERRAIN_RESOLUTION * TERRAIN_RESOLUTION, 0.0f);

    // Terrain mesh will be created by Terrain class
    // cell->terrain = std::make_shared<Terrain>();
    // return cell->terrain->createFlatTerrain(cell->cellX, cell->cellY);

    LOGD_CELL("Terrain mesh created for cell %u", cell->cellId);
    return true;
}

bool CellManager::loadNpcsForCell(std::shared_ptr<Cell> cell) {
    if (!cell) return false;
    // Implemented in Phase 3-4
    return true;
}

bool CellManager::loadObjectsForCell(std::shared_ptr<Cell> cell) {
    if (!cell) return false;
    // Implemented in Phase 3-4
    return true;
}

void CellManager::destroyTerrainMesh(std::shared_ptr<Cell> cell) {
    if (!cell) return;

    cell->terrain = nullptr;
    cell->heightData.clear();
}

// ============================================================================
// Cell Coordinate Utils
// ============================================================================

namespace CellCoordUtils {
    CellCoord getCoordFromWorldPos(const glm::vec3& worldPos) {
        int32_t cellX = static_cast<int32_t>(worldPos.x / CELL_SIZE);
        int32_t cellY = static_cast<int32_t>(worldPos.y / CELL_SIZE);

        // Handle negative coordinates
        if (worldPos.x < 0.0f && std::fmod(worldPos.x, CELL_SIZE) != 0.0f) {
            cellX--;
        }
        if (worldPos.y < 0.0f && std::fmod(worldPos.y, CELL_SIZE) != 0.0f) {
            cellY--;
        }

        return CellCoord(cellX, cellY);
    }

    glm::vec3 getWorldPosFromCoord(const CellCoord& coord) {
        return glm::vec3(
            coord.x * CELL_SIZE,
            coord.y * CELL_SIZE,
            0.0f
        );
    }

    std::vector<CellCoord> getAdjacentCoords(const CellCoord& center) {
        std::vector<CellCoord> adjacent;

        // 3x3 grid including center
        for (int32_t dx = -1; dx <= 1; dx++) {
            for (int32_t dy = -1; dy <= 1; dy++) {
                adjacent.push_back(CellCoord(center.x + dx, center.y + dy));
            }
        }

        return adjacent;
    }

    float getDistanceBetweenCoords(const CellCoord& a, const CellCoord& b) {
        int32_t dx = b.x - a.x;
        int32_t dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy) * CELL_SIZE;
    }

    bool isPosWithinCell(const glm::vec3& pos, const CellCoord& cellCoord) {
        glm::vec3 cellOrigin = getWorldPosFromCoord(cellCoord);
        return pos.x >= cellOrigin.x && pos.x < cellOrigin.x + CELL_SIZE &&
               pos.y >= cellOrigin.y && pos.y < cellOrigin.y + CELL_SIZE;
    }
}
