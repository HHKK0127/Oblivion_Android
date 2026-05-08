#pragma once

#include "world_data.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

// Forward declarations
class Mesh;
class ShaderProgram;

// ============================================================================
// Terrain - Heightmap-based terrain rendering
// ============================================================================

class Terrain {
public:
    Terrain();
    ~Terrain();

    // ========================================================================
    // Initialization
    // ========================================================================

    // Create terrain from heightmap data
    bool createFromHeightmap(const std::vector<float>& heightData,
                            int32_t cellX, int32_t cellY);

    // Create flat terrain (for testing)
    bool createFlatTerrain(int32_t cellX, int32_t cellY);

    // ========================================================================
    // Rendering
    // ========================================================================

    void render(const glm::mat4& viewProj, ShaderProgram* shader);

    // ========================================================================
    // Queries
    // ========================================================================

    // Get terrain height at local position (within cell)
    float getHeightAt(float localX, float localZ) const;

    // Get terrain height with interpolation
    float getInterpolatedHeight(float localX, float localZ) const;

    // Get terrain normal at position
    glm::vec3 getNormalAt(float localX, float localZ) const;

    // Check if position is valid within terrain
    bool isValidPosition(float localX, float localZ) const;

    // ========================================================================
    // Getters
    // ========================================================================

    const std::vector<float>& getHeightData() const { return heightData; }
    std::shared_ptr<Mesh> getMesh() const { return terrainMesh; }
    int32_t getCellX() const { return cellX; }
    int32_t getCellY() const { return cellY; }

    // ========================================================================
    // Constants
    // ========================================================================

    static constexpr int TERRAIN_GRID_SIZE = 65;  // 65x65

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    std::vector<float> heightData;          // 65x65 = 4225 height values
    std::shared_ptr<Mesh> terrainMesh;      // Rendered mesh
    int32_t cellX, cellY;                   // Cell coordinates
    glm::vec3 basePosition;                 // Cell world origin

    bool isInitialized;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Generate mesh from height data
    bool generateMeshFromHeightmap();

    // Normalize height values
    void normalizeHeights();

    // Calculate vertex normals
    void calculateVertexNormals();

    // Helper: Clamp local coordinates
    void clampLocalCoords(float& localX, float& localZ) const;

    // Helper: Get height at grid index
    float getHeightAtIndex(int32_t gridX, int32_t gridZ) const;
};
