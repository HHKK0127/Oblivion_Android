#pragma once

#include "world_data.h"
#include <android/log.h>

#define LOG_TAG_CELL "Cell"
#define LOGD_CELL(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_CELL, __VA_ARGS__)
#define LOGI_CELL(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_CELL, __VA_ARGS__)
#define LOGW_CELL(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_CELL, __VA_ARGS__)
#define LOGE_CELL(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_CELL, __VA_ARGS__)

// ============================================================================
// Cell Manager - Handles individual cell lifecycle
// ============================================================================

class CellManager {
public:
    CellManager();
    ~CellManager();

    // Initialization
    bool initialize(void* npcMgr = nullptr, void* assetMgr = nullptr);

    // Lifecycle
    bool loadCell(std::shared_ptr<Cell> cell);
    void unloadCell(std::shared_ptr<Cell> cell);
    void updateCell(std::shared_ptr<Cell> cell, float deltaTime);
    void renderCell(std::shared_ptr<Cell> cell);

    // Cell queries
    bool isCellLoaded(uint32_t cellId) const;
    std::vector<NPC*> getNpcsInCell(std::shared_ptr<Cell> cell);
    std::vector<WorldObject*> getObjectsInCell(std::shared_ptr<Cell> cell);

    // Memory management
    size_t calculateCellMemoryUsage(std::shared_ptr<Cell> cell);
    void compactCellMemory(std::shared_ptr<Cell> cell);

private:
    // Helper methods
    bool createTerrainMesh(std::shared_ptr<Cell> cell);
    bool loadNpcsForCell(std::shared_ptr<Cell> cell);
    bool loadObjectsForCell(std::shared_ptr<Cell> cell);
    void destroyTerrainMesh(std::shared_ptr<Cell> cell);
};

// ============================================================================
// Cell Coordinate Utils
// ============================================================================

namespace CellCoordUtils {
    // Convert world position to cell coordinates
    CellCoord getCoordFromWorldPos(const glm::vec3& worldPos);

    // Convert cell coordinates to world position (cell origin)
    glm::vec3 getWorldPosFromCoord(const CellCoord& coord);

    // Get all adjacent cells (8 neighbors + center = 9 total)
    std::vector<CellCoord> getAdjacentCoords(const CellCoord& center);

    // Calculate distance between cell coordinates
    float getDistanceBetweenCoords(const CellCoord& a, const CellCoord& b);

    // Check if position is within cell
    bool isPosWithinCell(const glm::vec3& pos, const CellCoord& cellCoord);
}
