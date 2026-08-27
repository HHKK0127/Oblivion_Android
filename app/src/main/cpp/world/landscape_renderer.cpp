#include "landscape_renderer.h"
#include "../assets/asset_manager.h"
#include "../geometry/mesh.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// LandscapeRenderer Implementation
// ============================================================================

LandscapeRenderer::LandscapeRenderer()
    : assetManager(nullptr), isInitialized(false) {
}

LandscapeRenderer::~LandscapeRenderer() {
    cleanup();
}

bool LandscapeRenderer::initialize(AssetManager* assetMgr) {
    assetManager = assetMgr;
    isInitialized = true;

    LOGI_LAND("LandscapeRenderer initialized");
    return true;
}

void LandscapeRenderer::cleanup() {
    clearCache();
    isInitialized = false;
    assetManager = nullptr;

    LOGI_LAND("LandscapeRenderer cleaned up");
}

// ============================================================================
// Terrain Generation from ESM Data
// ============================================================================

bool LandscapeRenderer::generateTerrainFromLAND(std::shared_ptr<Cell> cell,
                                                 const oblivion::TerrainData& landData) {
    if (!cell) {
        LOGE_LAND("Cannot generate terrain for null cell");
        return false;
    }

    if (!landData.hasHeights()) {
        LOGW_LAND("LAND record has no height data for cell 0x%08X", cell->cellId);
        return false;
    }

    LOGD_LAND("Generating terrain from LAND for cell %u (%d, %d)",
              cell->cellId, cell->cellX, cell->cellY);

    // Store height data in cell
    cell->heightData = landData.heights;

    // Generate mesh
    auto mesh = generateMesh(landData.heights, cell->cellX, cell->cellY);
    if (!mesh) {
        LOGE_LAND("Failed to generate terrain mesh for cell %u", cell->cellId);
        return false;
    }

    // Cache the mesh
    terrainCache[cell->cellId] = mesh;

    // Calculate and cache normals
    auto normals = calculateNormals(landData.heights, GRID_SIZE);
    normalCache[cell->cellId] = normals;

    LOGI_LAND("Terrain generated for cell %u: %zu vertices, %zu normals",
              cell->cellId, landData.heights.size(), normals.size());

    return true;
}

bool LandscapeRenderer::generateCellTerrain(std::shared_ptr<Cell> cell,
                                             const oblivion::ESMManager& esmMgr) {
    if (!cell) {
        LOGE_LAND("Cannot generate terrain for null cell");
        return false;
    }

    // Find terrain data for this cell
    const auto& terrains = esmMgr.getAllTerrains();
    const oblivion::TerrainData* landData = nullptr;

    for (const auto& terrain : terrains) {
        if (terrain.formID == cell->tesFormID) {
            landData = &terrain;
            break;
        }
    }

    if (!landData) {
        LOGD_LAND("No LAND data for cell %u, generating flat terrain", cell->cellId);
        // Generate flat terrain as fallback
        std::vector<float> flatHeights(GRID_SIZE * GRID_SIZE, 0.0f);
        cell->heightData = flatHeights;

        auto mesh = generateMesh(flatHeights, cell->cellX, cell->cellY);
        if (mesh) {
            terrainCache[cell->cellId] = mesh;
        }
        return true;
    }

    return generateTerrainFromLAND(cell, *landData);
}

// ============================================================================
// Heightmap Processing
// ============================================================================

std::vector<float> LandscapeRenderer::parseHeightmap(const std::vector<uint8_t>& rawData) {
    std::vector<float> heights;

    if (rawData.size() < GRID_SIZE * GRID_SIZE * sizeof(int16_t)) {
        LOGE_LAND("Raw heightmap data too small: %zu bytes", rawData.size());
        return heights;
    }

    heights.reserve(GRID_SIZE * GRID_SIZE);

    // LAND records store heights as int16_t values
    const int16_t* rawHeights = reinterpret_cast<const int16_t*>(rawData.data());

    for (int32_t i = 0; i < GRID_SIZE * GRID_SIZE; ++i) {
        // Convert int16 to float and apply scale
        float height = static_cast<float>(rawHeights[i]) * (1.0f / 64.0f);
        heights.push_back(height);
    }

    LOGD_LAND("Parsed heightmap: %zu vertices", heights.size());
    return heights;
}

std::vector<glm::vec3> LandscapeRenderer::calculateNormals(const std::vector<float>& heights,
                                                            int32_t gridSize) {
    std::vector<glm::vec3> normals(heights.size(), glm::vec3(0.0f, 1.0f, 0.0f));

    if (heights.size() != static_cast<size_t>(gridSize * gridSize)) {
        LOGE_LAND("Invalid heightmap size for normal calculation");
        return normals;
    }

    float cellStep = CELL_WORLD_SIZE / (gridSize - 1);

    for (int32_t z = 0; z < gridSize; ++z) {
        for (int32_t x = 0; x < gridSize; ++x) {
            int32_t idx = z * gridSize + x;

            // Get heights of neighboring vertices
            float hL = (x > 0) ? heights[z * gridSize + (x - 1)] : heights[idx];
            float hR = (x < gridSize - 1) ? heights[z * gridSize + (x + 1)] : heights[idx];
            float hD = (z > 0) ? heights[(z - 1) * gridSize + x] : heights[idx];
            float hU = (z < gridSize - 1) ? heights[(z + 1) * gridSize + x] : heights[idx];

            // Calculate normal using central differences
            glm::vec3 normal;
            normal.x = (hL - hR) / (2.0f * cellStep);
            normal.z = (hD - hU) / (2.0f * cellStep);
            normal.y = 1.0f;

            // Normalize
            float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            if (len > 0.0001f) {
                normal /= len;
            }

            normals[idx] = normal;
        }
    }

    return normals;
}

float LandscapeRenderer::getHeightAt(const std::vector<float>& heights,
                                       float localX, float localZ, int32_t gridSize) {
    if (heights.empty()) return 0.0f;

    // Convert local coordinates to grid coordinates
    float gridX = (localX / CELL_WORLD_SIZE) * (gridSize - 1);
    float gridZ = (localZ / CELL_WORLD_SIZE) * (gridSize - 1);

    // Clamp to valid range
    gridX = std::max(0.0f, std::min(static_cast<float>(gridSize - 2), gridX));
    gridZ = std::max(0.0f, std::min(static_cast<float>(gridSize - 2), gridZ));

    int32_t x0 = static_cast<int32_t>(gridX);
    int32_t z0 = static_cast<int32_t>(gridZ);
    int32_t x1 = x0 + 1;
    int32_t z1 = z0 + 1;

    float fracX = gridX - x0;
    float fracZ = gridZ - z0;

    // Bilinear interpolation
    float h00 = heights[z0 * gridSize + x0];
    float h10 = heights[z0 * gridSize + x1];
    float h01 = heights[z1 * gridSize + x0];
    float h11 = heights[z1 * gridSize + x1];

    float h0 = h00 * (1.0f - fracX) + h10 * fracX;
    float h1 = h01 * (1.0f - fracX) + h11 * fracX;

    return h0 * (1.0f - fracZ) + h1 * fracZ;
}

// ============================================================================
// Texture Blend Layers
// ============================================================================

std::vector<LandscapeRenderer::TextureLayer> LandscapeRenderer::parseTextureLayers(
    const oblivion::ESMManager& esmMgr, uint32_t cellFormID) {
    std::vector<TextureLayer> layers;

    // LAND records contain BTXT (base texture) and ATXT (alpha texture) subrecords
    // For now, return empty layers - full texture blending requires BSA texture loading
    LOGD_LAND("Texture layer parsing for cell 0x%08X (placeholder)", cellFormID);

    return layers;
}

// ============================================================================
// Rendering
// ============================================================================

void LandscapeRenderer::renderCell(std::shared_ptr<Cell> cell,
                                    const glm::mat4& viewProj,
                                    ShaderProgram* shader) {
    if (!cell || !shader) return;

    auto it = terrainCache.find(cell->cellId);
    if (it == terrainCache.end()) {
        LOGW_LAND("No cached terrain mesh for cell %u", cell->cellId);
        return;
    }

    auto& mesh = it->second;
    if (mesh) {
        // Render the terrain mesh
        // The actual rendering is handled by the renderer system
        LOGD_LAND("Rendering terrain for cell %u", cell->cellId);
    }
}

void LandscapeRenderer::renderAll(const glm::mat4& viewProj,
                                    ShaderProgram* shader) {
    if (!shader) return;

    for (auto& pair : terrainCache) {
        if (pair.second) {
            // Render each terrain mesh
            LOGD_LAND("Rendering terrain mesh for cell %u", pair.first);
        }
    }
}

// ============================================================================
// Mesh Generation
// ============================================================================

std::shared_ptr<Mesh> LandscapeRenderer::generateMesh(const std::vector<float>& heights,
                                                       int32_t cellX, int32_t cellY) {
    if (heights.size() != static_cast<size_t>(GRID_SIZE * GRID_SIZE)) {
        LOGE_LAND("Invalid heightmap size: %zu (expected %d)",
                  heights.size(), GRID_SIZE * GRID_SIZE);
        return nullptr;
    }

    // Generate vertex positions
    auto positions = generateVertices(heights, cellX, cellY);

    // Generate UVs
    auto uvs = generateUVs(GRID_SIZE);

    // Calculate normals
    auto normals = calculateNormals(heights, GRID_SIZE);

    // Generate indices
    auto rawIndices = generateIndices(GRID_SIZE);

    // Build Vertex structs for Mesh API
    std::vector<Vertex> meshVertices;
    meshVertices.reserve(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex v;
        v.position = positions[i];
        v.normal = (i < normals.size()) ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
        v.texCoord = (i < uvs.size()) ? uvs[i] : glm::vec2(0.0f, 0.0f);
        v.color = glm::vec3(1.0f, 1.0f, 1.0f);
        meshVertices.push_back(v);
    }

    // Convert indices to unsigned int
    std::vector<unsigned int> meshIndices(rawIndices.begin(), rawIndices.end());

    // Create mesh
    auto mesh = std::make_shared<Mesh>();
    mesh->setVertices(meshVertices);
    mesh->setIndices(meshIndices);

    LOGD_LAND("Generated terrain mesh for cell (%d, %d): %zu vertices, %zu triangles",
              cellX, cellY, meshVertices.size(), meshIndices.size() / 3);

    return mesh;
}

std::vector<glm::vec3> LandscapeRenderer::generateVertices(const std::vector<float>& heights,
                                                            int32_t cellX, int32_t cellY) {
    std::vector<glm::vec3> vertices;
    vertices.reserve(heights.size());

    float cellStep = CELL_WORLD_SIZE / (GRID_SIZE - 1);

    // Cell world origin
    float originX = static_cast<float>(cellX) * CELL_WORLD_SIZE;
    float originZ = static_cast<float>(cellY) * CELL_WORLD_SIZE;

    for (int32_t z = 0; z < GRID_SIZE; ++z) {
        for (int32_t x = 0; x < GRID_SIZE; ++x) {
            int32_t idx = z * GRID_SIZE + x;

            float posX = originX + x * cellStep;
            float posY = heights[idx] * HEIGHT_SCALE;
            float posZ = originZ + z * cellStep;

            vertices.emplace_back(posX, posY, posZ);
        }
    }

    return vertices;
}

std::vector<glm::vec2> LandscapeRenderer::generateUVs(int32_t gridSize) {
    std::vector<glm::vec2> uvs;
    uvs.reserve(gridSize * gridSize);

    float uvStep = 1.0f / (gridSize - 1);

    for (int32_t z = 0; z < gridSize; ++z) {
        for (int32_t x = 0; x < gridSize; ++x) {
            uvs.emplace_back(x * uvStep, z * uvStep);
        }
    }

    return uvs;
}

std::vector<uint32_t> LandscapeRenderer::generateIndices(int32_t gridSize) {
    std::vector<uint32_t> indices;

    for (int32_t z = 0; z < gridSize - 1; ++z) {
        for (int32_t x = 0; x < gridSize - 1; ++x) {
            uint32_t topLeft = z * gridSize + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * gridSize + x;
            uint32_t bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return indices;
}

void LandscapeRenderer::calculateSmoothNormals(std::vector<glm::vec3>& normals,
                                                 const std::vector<glm::vec3>& vertices,
                                                 const std::vector<uint32_t>& indices) {
    // Initialize normals to zero
    normals.assign(vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));

    // Accumulate face normals for each vertex
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }

        const glm::vec3& v0 = vertices[i0];
        const glm::vec3& v1 = vertices[i1];
        const glm::vec3& v2 = vertices[i2];

        // Calculate face normal
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        // Accumulate (area-weighted)
        normals[i0] += faceNormal;
        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
    }

    // Normalize
    for (auto& normal : normals) {
        float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0.0001f) {
            normal /= len;
        } else {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

// ============================================================================
// Cache Management
// ============================================================================

std::shared_ptr<Mesh> LandscapeRenderer::getCachedMesh(uint32_t cellId) const {
    auto it = terrainCache.find(cellId);
    if (it != terrainCache.end()) {
        return it->second;
    }
    return nullptr;
}

void LandscapeRenderer::clearCache() {
    terrainCache.clear();
    normalCache.clear();
    LOGD_LAND("Terrain cache cleared");
}
