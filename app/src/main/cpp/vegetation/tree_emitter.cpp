// ============================================================================
// TreeEmitter - ESM LAND record-based tree placement generator
// Phase 51: Generates tree positions from ESM data with natural distribution
// ============================================================================

#include "tree_emitter.h"
#include "speed_tree_manager.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <android/log.h>

#undef LOG_TAG
#define LOG_TAG "TreeEmitter"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace vegetation {

// ============================================================================
// Permutation table (same as WindField for consistency)
// ============================================================================

static const int EMITTER_PERM[256] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

TreeEmitter::TreeEmitter() {
    std::memset(perm_, 0, sizeof(perm_));
}

TreeEmitter::~TreeEmitter() = default;

void TreeEmitter::initialize(uint32_t seed) {
    seed_ = seed;

    for (int i = 0; i < 256; i++) {
        perm_[i] = EMITTER_PERM[i];
    }

    // Seed-based shuffle
    uint32_t s = seed;
    for (int i = 255; i > 0; i--) {
        s = s * 1103515245 + 12345;
        int j = static_cast<int>((s >> 16) % static_cast<uint32_t>(i + 1));
        if (j < 0) j = -j;
        std::swap(perm_[i], perm_[j]);
    }

    for (int i = 0; i < 256; i++) {
        perm_[256 + i] = perm_[i];
    }
}

// ============================================================================
// Noise functions
// ============================================================================

float TreeEmitter::fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float TreeEmitter::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float TreeEmitter::grad(int hash, float x, float y) const {
    int h = hash & 3;
    float u = (h < 2) ? x : y;
    float v = (h < 2) ? y : x;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float TreeEmitter::noise2D(float x, float y) const {
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    float u = fade(xf);
    float v = fade(yf);

    int a  = perm_[xi] + yi;
    int b  = perm_[xi + 1] + yi;

    return lerp(
        lerp(grad(perm_[a], xf, yf),
             grad(perm_[b], xf - 1, yf), u),
        lerp(grad(perm_[a + 1], xf, yf - 1),
             grad(perm_[b + 1], xf - 1, yf - 1), u), v);
}

float TreeEmitter::densityNoise(float worldX, float worldZ) const {
    // Multi-octave noise for natural clustering
    float scale1 = 0.001f;   // Large-scale density variation
    float scale2 = 0.005f;   // Medium-scale clustering
    float scale3 = 0.02f;    // Small-scale variation

    float n1 = noise2D(worldX * scale1, worldZ * scale1) * 0.5f;
    float n2 = noise2D(worldX * scale2, worldZ * scale2) * 0.3f;
    float n3 = noise2D(worldX * scale3, worldZ * scale3) * 0.2f;

    // Normalize to [0, 1]
    float value = (n1 + n2 + n3 + 1.0f) * 0.5f;
    return std::max(0.0f, std::min(1.0f, value));
}

uint32_t TreeEmitter::hashPosition(int32_t x, int32_t z) const {
    // Simple hash for deterministic placement
    uint32_t h = static_cast<uint32_t>(seed_);
    h ^= static_cast<uint32_t>(x) * 374761393u;
    h ^= static_cast<uint32_t>(z) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h = h ^ (h >> 16);
    return h;
}

// ============================================================================
// Terrain sampling
// ============================================================================

float TreeEmitter::sampleTerrainHeight(const LandCellData& cell,
                                        float localX, float localY) const {
    if (!cell.hasData) return 0.0f;

    // Map local coordinates [0, cellSize] to heightmap grid [0, 32]
    float gridX = (localX / cell.cellSize) * 32.0f;
    float gridY = (localY / cell.cellSize) * 32.0f;

    // Clamp to valid range
    gridX = std::max(0.0f, std::min(32.0f, gridX));
    gridY = std::max(0.0f, std::min(32.0f, gridY));

    int ix = static_cast<int>(gridX);
    int iy = static_cast<int>(gridY);
    float fx = gridX - static_cast<float>(ix);
    float fy = gridY - static_cast<float>(iy);

    // Clamp indices
    ix = std::min(ix, 31);
    iy = std::min(iy, 31);

    // Bilinear interpolation
    float h00 = cell.heightmap[iy][ix];
    float h10 = cell.heightmap[iy][ix + 1];
    float h01 = cell.heightmap[iy + 1][ix];
    float h11 = cell.heightmap[iy + 1][ix + 1];

    float h0 = lerp(h00, h10, fx);
    float h1 = lerp(h01, h11, fx);

    return lerp(h0, h1, fy);
}

float TreeEmitter::calculateSlope(const LandCellData& cell,
                                   float localX, float localY) const {
    if (!cell.hasData) return 0.0f;

    float step = cell.cellSize / 32.0f;
    float hL = sampleTerrainHeight(cell, localX - step, localY);
    float hR = sampleTerrainHeight(cell, localX + step, localY);
    float hD = sampleTerrainHeight(cell, localX, localY - step);
    float hU = sampleTerrainHeight(cell, localX, localY + step);

    float dx = (hR - hL) / (2.0f * step);
    float dy = (hU - hD) / (2.0f * step);

    // Slope in degrees
    float slopeRad = std::atan(std::sqrt(dx * dx + dy * dy));
    return slopeRad * 180.0f / 3.14159265f;
}

// ============================================================================
// Placement validation
// ============================================================================

bool TreeEmitter::isValidPlacement(const glm::vec3& pos,
                                    const std::vector<TreeInstance>& existingTrees,
                                    float minDistance) const {
    float minDistSq = minDistance * minDistance;

    for (const auto& tree : existingTrees) {
        glm::vec3 diff = tree.position - pos;
        float distSq = diff.x * diff.x + diff.z * diff.z;
        if (distSq < minDistSq) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Tree generation for a single cell
// ============================================================================

std::vector<TreeInstance> TreeEmitter::generateTreesForCell(
    const LandCellData& cell,
    const TreeDistributionConfig& config) const {

    std::vector<TreeInstance> trees;

    if (!cell.hasData) return trees;

    // World-space origin of this cell
    float worldOriginX = static_cast<float>(cell.cellX) * cell.cellSize;
    float worldOriginZ = static_cast<float>(cell.cellY) * cell.cellSize;

    // Calculate number of candidate positions based on density
    float cellArea = cell.cellSize * cell.cellSize;
    float areaHectares = cellArea / 10000.0f;  // Rough conversion
    int numCandidates = static_cast<int>(areaHectares * config.density * densityMultiplier_ * 10.0f);
    numCandidates = std::max(1, std::min(numCandidates, 500));

    // Deterministic random based on cell position
    uint32_t cellHash = hashPosition(cell.cellX, cell.cellY);
    uint32_t rng = cellHash;

    for (int i = 0; i < numCandidates; i++) {
        // Generate pseudo-random position within cell
        rng = rng * 1103515245 + 12345;
        float localX = (static_cast<float>((rng >> 16) & 0xFFFF) / 65535.0f) * cell.cellSize;

        rng = rng * 1103515245 + 12345;
        float localZ = (static_cast<float>((rng >> 16) & 0xFFFF) / 65535.0f) * cell.cellSize;

        float worldX = worldOriginX + localX;
        float worldZ = worldOriginZ + localZ;

        // Density noise check (natural clustering)
        float density = densityNoise(worldX, worldZ);
        rng = rng * 1103515245 + 12345;
        float randomCheck = static_cast<float>((rng >> 16) & 0xFFFF) / 65535.0f;
        if (randomCheck > density) continue;

        // Sample terrain height
        float height = sampleTerrainHeight(cell, localX, localZ);

        // Altitude check
        if (height < config.altitudeMin || height > config.altitudeMax) continue;

        // Slope check
        float slope = calculateSlope(cell, localX, localZ);
        if (slope > config.terrainSlopeMax) continue;

        // Distance check against already-placed trees
        if (!isValidPlacement(glm::vec3(worldX, height, worldZ), trees, config.minDistance)) {
            continue;
        }

        // Generate tree instance
        TreeInstance tree;
        rng = rng * 1103515245 + 12345;
        tree.typeId = config.defaultTreeType;
        tree.position = glm::vec3(worldX, height, worldZ);

        rng = rng * 1103515245 + 12345;
        tree.rotation = (static_cast<float>((rng >> 16) & 0xFFFF) / 65535.0f) * 2.0f * 3.14159265f;

        rng = rng * 1103515245 + 12345;
        float scaleRange = config.maxScale - config.minScale;
        tree.scale = config.minScale +
            (static_cast<float>((rng >> 16) & 0xFFFF) / 65535.0f) * scaleRange;

        trees.push_back(tree);
    }

    return trees;
}

// ============================================================================
// Tree generation for an area of cells
// ============================================================================

std::vector<TreeInstance> TreeEmitter::generateTreesForArea(
    int32_t cellMinX, int32_t cellMinY,
    int32_t cellMaxX, int32_t cellMaxY,
    const TreeDistributionConfig& config) const {

    std::vector<TreeInstance> allTrees;

    for (int32_t cy = cellMinY; cy <= cellMaxY; cy++) {
        for (int32_t cx = cellMinX; cx <= cellMaxX; cx++) {
            // Create a placeholder cell (in real implementation,
            // this would load from ESM data)
            LandCellData cell;
            cell.cellX = cx;
            cell.cellY = cy;
            cell.hasData = false;

            // Generate trees for this cell
            auto cellTrees = generateTreesForCell(cell, config);

            // Merge with global distance checking
            for (auto& tree : cellTrees) {
                if (isValidPlacement(tree.position, allTrees, config.minDistance)) {
                    allTrees.push_back(tree);
                }
            }
        }
    }

    LOGI("Generated %lu trees for area [%d,%d]-[%d,%d]",
         (unsigned long)allTrees.size(),
         cellMinX, cellMinY, cellMaxX, cellMaxY);

    return allTrees;
}

} // namespace vegetation
