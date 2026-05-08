#include "grid.h"
#include "world_data.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// CellGrid Implementation
// ============================================================================

CellGrid::CellGrid() {
}

CellGrid::~CellGrid() {
    clear();
}

void CellGrid::registerCell(uint32_t cellId, int32_t cellX, int32_t cellY) {
    uint64_t key = coordsToKey(cellX, cellY);
    coordToCellId[key] = cellId;
    cellIdToCoord[cellId] = std::make_pair(cellX, cellY);
}

void CellGrid::unregisterCell(uint32_t cellId) {
    auto it = cellIdToCoord.find(cellId);
    if (it != cellIdToCoord.end()) {
        uint64_t key = coordsToKey(it->second.first, it->second.second);
        coordToCellId.erase(key);
        cellIdToCoord.erase(it);
    }
}

void CellGrid::unregisterCell(int32_t cellX, int32_t cellY) {
    uint64_t key = coordsToKey(cellX, cellY);
    auto it = coordToCellId.find(key);
    if (it != coordToCellId.end()) {
        cellIdToCoord.erase(it->second);
        coordToCellId.erase(it);
    }
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

void CellGrid::worldPosToCellCoord(const glm::vec3& worldPos,
                                  int32_t& outCellX, int32_t& outCellY) {
    outCellX = static_cast<int32_t>(std::floor(worldPos.x / CELL_SIZE));
    outCellY = static_cast<int32_t>(std::floor(worldPos.y / CELL_SIZE));
}

glm::vec3 CellGrid::cellCoordToWorldPos(int32_t cellX, int32_t cellY) {
    return glm::vec3(
        cellX * CELL_SIZE,
        cellY * CELL_SIZE,
        0.0f
    );
}

glm::vec3 CellGrid::worldPosToLocalPos(const glm::vec3& worldPos) {
    int32_t cellX, cellY;
    worldPosToCellCoord(worldPos, cellX, cellY);

    glm::vec3 cellOrigin = cellCoordToWorldPos(cellX, cellY);
    return worldPos - cellOrigin;
}

// ============================================================================
// Queries
// ============================================================================

uint32_t CellGrid::getCellIdAt(const glm::vec3& worldPos) {
    int32_t cellX, cellY;
    worldPosToCellCoord(worldPos, cellX, cellY);
    return getCellId(cellX, cellY);
}

uint32_t CellGrid::getCellId(int32_t cellX, int32_t cellY) {
    uint64_t key = coordsToKey(cellX, cellY);
    auto it = coordToCellId.find(key);
    if (it != coordToCellId.end()) {
        return it->second;
    }
    return 0;  // Invalid cell ID
}

bool CellGrid::getCoords(uint32_t cellId, int32_t& outX, int32_t& outY) {
    auto it = cellIdToCoord.find(cellId);
    if (it != cellIdToCoord.end()) {
        outX = it->second.first;
        outY = it->second.second;
        return true;
    }
    return false;
}

std::vector<uint32_t> CellGrid::getAdjacentCellIds(uint32_t cellId) {
    int32_t cellX, cellY;
    if (!getCoords(cellId, cellX, cellY)) {
        return {};
    }
    return getAdjacentCellIds(cellX, cellY);
}

std::vector<uint32_t> CellGrid::getAdjacentCellIds(int32_t cellX, int32_t cellY) {
    std::vector<uint32_t> adjacent;

    // 3x3 grid
    for (int32_t dx = -1; dx <= 1; dx++) {
        for (int32_t dy = -1; dy <= 1; dy++) {
            uint32_t cellId = getCellId(cellX + dx, cellY + dy);
            if (cellId != 0) {
                adjacent.push_back(cellId);
            }
        }
    }

    return adjacent;
}

std::vector<uint32_t> CellGrid::getCellsInRadius(const glm::vec3& center, float radius) {
    std::vector<uint32_t> result;

    int32_t centerCellX, centerCellY;
    worldPosToCellCoord(center, centerCellX, centerCellY);

    // Calculate cell radius
    int32_t cellRadius = static_cast<int32_t>(std::ceil(radius / CELL_SIZE)) + 1;

    for (int32_t dx = -cellRadius; dx <= cellRadius; dx++) {
        for (int32_t dy = -cellRadius; dy <= cellRadius; dy++) {
            int32_t checkX = centerCellX + dx;
            int32_t checkY = centerCellY + dy;

            glm::vec3 cellPos = cellCoordToWorldPos(checkX, checkY);
            glm::vec3 diff = center - cellPos;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

            if (dist <= radius) {
                uint32_t cellId = getCellId(checkX, checkY);
                if (cellId != 0) {
                    result.push_back(cellId);
                }
            }
        }
    }

    return result;
}

// ============================================================================
// Utility
// ============================================================================

float CellGrid::getDistanceBetweenCells(uint32_t cellId1, uint32_t cellId2) {
    int32_t x1, y1, x2, y2;

    if (!getCoords(cellId1, x1, y1) || !getCoords(cellId2, x2, y2)) {
        return FLT_MAX;
    }

    return getDistanceBetweenCoords(x1, y1, x2, y2);
}

float CellGrid::getDistanceBetweenCoords(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    float dx = static_cast<float>(x2 - x1);
    float dy = static_cast<float>(y2 - y1);
    return std::sqrt(dx * dx + dy * dy) * CELL_SIZE;
}

bool CellGrid::isPosWithinCell(const glm::vec3& pos, int32_t cellX, int32_t cellY) {
    glm::vec3 cellOrigin = cellCoordToWorldPos(cellX, cellY);
    return pos.x >= cellOrigin.x && pos.x < cellOrigin.x + CELL_SIZE &&
           pos.y >= cellOrigin.y && pos.y < cellOrigin.y + CELL_SIZE;
}

void CellGrid::clear() {
    coordToCellId.clear();
    cellIdToCoord.clear();
}

// ============================================================================
// Private Methods
// ============================================================================

uint64_t CellGrid::coordsToKey(int32_t x, int32_t y) {
    // Pack two int32 values into uint64
    return (static_cast<uint64_t>(x) << 32) | (static_cast<uint32_t>(y));
}

void CellGrid::keyToCoords(uint64_t key, int32_t& outX, int32_t& outY) {
    outX = static_cast<int32_t>(key >> 32);
    outY = static_cast<int32_t>(key & 0xFFFFFFFF);
}
