#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

// ============================================================================
// Cell Grid - Spatial indexing and fast lookups
// ============================================================================

class CellGrid {
public:
    CellGrid();
    ~CellGrid();

    // ========================================================================
    // Registration
    // ========================================================================

    // Register a cell in the grid
    void registerCell(uint32_t cellId, int32_t cellX, int32_t cellY);

    // Unregister a cell
    void unregisterCell(uint32_t cellId);
    void unregisterCell(int32_t cellX, int32_t cellY);

    // ========================================================================
    // Coordinate Conversion
    // ========================================================================

    // Convert world position to cell coordinates
    static void worldPosToCellCoord(const glm::vec3& worldPos,
                                   int32_t& outCellX, int32_t& outCellY);

    // Convert cell coordinates to world position (cell origin)
    static glm::vec3 cellCoordToWorldPos(int32_t cellX, int32_t cellY);

    // Convert world position to local position within cell
    static glm::vec3 worldPosToLocalPos(const glm::vec3& worldPos);

    // ========================================================================
    // Queries
    // ========================================================================

    // Get cell ID at world position
    uint32_t getCellIdAt(const glm::vec3& worldPos);
    uint32_t getCellId(int32_t cellX, int32_t cellY);

    // Get coordinates from cell ID
    bool getCoords(uint32_t cellId, int32_t& outX, int32_t& outY);

    // Get all adjacent cell IDs (8 neighbors)
    std::vector<uint32_t> getAdjacentCellIds(uint32_t cellId);
    std::vector<uint32_t> getAdjacentCellIds(int32_t cellX, int32_t cellY);

    // Get all cells in a radius
    std::vector<uint32_t> getCellsInRadius(const glm::vec3& center, float radius);

    // ========================================================================
    // Utility
    // ========================================================================

    // Calculate distance between cells
    float getDistanceBetweenCells(uint32_t cellId1, uint32_t cellId2);
    float getDistanceBetweenCoords(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

    // Check if position is within cell bounds
    static bool isPosWithinCell(const glm::vec3& pos, int32_t cellX, int32_t cellY);

    // Get size of the grid
    size_t getRegisteredCellCount() const { return coordToCellId.size(); }

    // Clear all registrations
    void clear();

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    // Map: (cellX, cellY) → cellId
    std::unordered_map<uint64_t, uint32_t> coordToCellId;

    // Map: cellId → (cellX, cellY)
    std::unordered_map<uint32_t, std::pair<int32_t, int32_t>> cellIdToCoord;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Create unique key from coordinates
    static uint64_t coordsToKey(int32_t x, int32_t y);

    // Extract coordinates from key
    static void keyToCoords(uint64_t key, int32_t& outX, int32_t& outY);
};
