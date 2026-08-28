#pragma once

// ============================================================================
// TreeEmitter - Esm LAND record-based tree placement generator
// Phase 51: Generates tree positions from ESM data with natural distribution
// ============================================================================

#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace vegetation {

struct TreeInstance;

// Configuration for tree distribution in a cell
struct TreeDistributionConfig {
    float density = 0.5f;           // Trees per 100 sq units
    float minScale = 0.7f;          // Minimum random scale
    float maxScale = 1.3f;          // Maximum random scale
    float minDistance = 3.0f;       // Minimum distance between trees
    float terrainSlopeMax = 45.0f;  // Max slope angle in degrees for placement
    float altitudeMin = -100.0f;    // Minimum altitude for tree placement
    float altitudeMax = 500.0f;     // Maximum altitude for tree placement
    uint32_t defaultTreeType = 1;   // Default tree type ID
};

// ESM LAND record data for a single cell
struct LandCellData {
    int32_t cellX = 0;
    int32_t cellY = 0;
    float cellSize = 4096.0f;       // Oblivion cell size in units
    // Heightmap data (33x33 grid for Oblivion LAND records)
    float heightmap[33][33] = {};
    // Vertex colors (33x33 RGB)
    uint8_t vertexColors[33][33][3] = {};
    // Texture layer indices
    uint8_t textureLayers[16][16] = {};
    bool hasData = false;
};

class TreeEmitter {
public:
    TreeEmitter();
    ~TreeEmitter();

    // Initialize with noise seed for reproducible generation
    void initialize(uint32_t seed = 12345);

    // Generate tree positions for a single LAND cell
    std::vector<TreeInstance> generateTreesForCell(
        const LandCellData& cell,
        const TreeDistributionConfig& config) const;

    // Generate trees for a rectangular area of cells
    std::vector<TreeInstance> generateTreesForArea(
        int32_t cellMinX, int32_t cellMinY,
        int32_t cellMaxX, int32_t cellMaxY,
        const TreeDistributionConfig& config) const;

    // Check if a position is valid (not too close to existing trees)
    bool isValidPlacement(const glm::vec3& pos,
                          const std::vector<TreeInstance>& existingTrees,
                          float minDistance) const;

    // Sample terrain height from LAND cell data
    float sampleTerrainHeight(const LandCellData& cell,
                              float localX, float localY) const;

    // Calculate terrain slope at a point
    float calculateSlope(const LandCellData& cell,
                         float localX, float localY) const;

    // Noise-based density variation (for natural clustering)
    float densityNoise(float worldX, float worldZ) const;

    // Set global density multiplier
    void setDensityMultiplier(float mult) { densityMultiplier_ = mult; }
    float getDensityMultiplier() const { return densityMultiplier_; }

private:
    // Perlin-like noise for distribution
    float noise2D(float x, float y) const;
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;

    // Hash function for deterministic placement
    uint32_t hashPosition(int32_t x, int32_t z) const;

    int perm_[512];
    uint32_t seed_ = 12345;
    float densityMultiplier_ = 1.0f;
};

} // namespace vegetation
