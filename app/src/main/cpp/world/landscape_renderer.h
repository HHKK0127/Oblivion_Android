#pragma once

#include "world_data.h"
#include "../assets/esm_reader.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <android/log.h>

// Forward declarations
class Mesh;
class ShaderProgram;
class AssetManager;

#define LOG_TAG_LANDSCAPE "LandscapeRenderer"
#define LOGD_LAND(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_LANDSCAPE, __VA_ARGS__)
#define LOGI_LAND(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_LANDSCAPE, __VA_ARGS__)
#define LOGW_LAND(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_LANDSCAPE, __VA_ARGS__)
#define LOGE_LAND(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_LANDSCAPE, __VA_ARGS__)

// ============================================================================
// Landscape Renderer - Renders terrain from ESM LAND records
// ============================================================================

class LandscapeRenderer {
public:
    LandscapeRenderer();
    ~LandscapeRenderer();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize(AssetManager* assetMgr);
    void cleanup();

    // ========================================================================
    // Terrain Generation from ESM Data
    // ========================================================================

    // Generate terrain mesh from LAND record heightmap
    bool generateTerrainFromLAND(std::shared_ptr<Cell> cell,
                                  const oblivion::TerrainData& landData);

    // Generate terrain for a cell using ESM terrain data
    bool generateCellTerrain(std::shared_ptr<Cell> cell,
                              const oblivion::ESMManager& esmMgr);

    // ========================================================================
    // Heightmap Processing
    // ========================================================================

    // Parse raw heightmap data from LAND record (VERT subrecord)
    std::vector<float> parseHeightmap(const std::vector<uint8_t>& rawData);

    // Calculate normals for terrain vertices
    std::vector<glm::vec3> calculateNormals(const std::vector<float>& heights,
                                             int32_t gridSize);

    // Interpolate height at arbitrary position
    float getHeightAt(const std::vector<float>& heights,
                      float localX, float localZ, int32_t gridSize);

    // ========================================================================
    // Texture Blend Layers
    // ========================================================================

    // Texture layer for terrain blending
    struct TextureLayer {
        uint32_t textureFormID = 0;
        std::string texturePath;
        std::vector<uint8_t> blendMap;  // Per-vertex blend weight
    };

    // Parse texture layers from LAND record
    std::vector<TextureLayer> parseTextureLayers(const oblivion::ESMManager& esmMgr,
                                                  uint32_t cellFormID);

    // ========================================================================
    // Rendering
    // ========================================================================

    // Render terrain for a cell
    void renderCell(std::shared_ptr<Cell> cell,
                    const glm::mat4& viewProj,
                    ShaderProgram* shader);

    // Render all loaded terrain
    void renderAll(const glm::mat4& viewProj,
                   ShaderProgram* shader);

    // ========================================================================
    // Mesh Generation
    // ========================================================================

    // Generate mesh from heightmap data
    std::shared_ptr<Mesh> generateMesh(const std::vector<float>& heights,
                                        int32_t cellX, int32_t cellY);

    // ========================================================================
    // Cache Management
    // ========================================================================

    // Get cached terrain mesh for a cell
    std::shared_ptr<Mesh> getCachedMesh(uint32_t cellId) const;

    // Clear terrain cache
    void clearCache();

    // Get cache size
    size_t getCacheSize() const { return terrainCache.size(); }

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    AssetManager* assetManager;
    bool isInitialized;

    // Terrain mesh cache: cellId -> mesh
    std::unordered_map<uint32_t, std::shared_ptr<Mesh>> terrainCache;

    // Normal cache: cellId -> normals
    std::unordered_map<uint32_t, std::vector<glm::vec3>> normalCache;

    // ========================================================================
    // Constants
    // ========================================================================

    static constexpr int32_t GRID_SIZE = 65;        // 65x65 vertices per cell
    static constexpr float CELL_WORLD_SIZE = 128.0f; // World units per cell
    static constexpr float HEIGHT_SCALE = 8.0f;     // Height multiplier for visualization

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Generate vertex positions from heightmap
    std::vector<glm::vec3> generateVertices(const std::vector<float>& heights,
                                             int32_t cellX, int32_t cellY);

    // Generate UV coordinates
    std::vector<glm::vec2> generateUVs(int32_t gridSize);

    // Generate triangle indices
    std::vector<uint32_t> generateIndices(int32_t gridSize);

    // Calculate smooth normals using area-weighted averaging
    void calculateSmoothNormals(std::vector<glm::vec3>& normals,
                                 const std::vector<glm::vec3>& vertices,
                                 const std::vector<uint32_t>& indices);
};
