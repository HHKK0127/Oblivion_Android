#include "terrain.h"
#include "../geometry/mesh.h"
#include "../engine/shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// ============================================================================
// Terrain Implementation
// ============================================================================

Terrain::Terrain()
    : cellX(0), cellY(0), basePosition(0.0f, 0.0f, 0.0f), isInitialized(false) {
    heightData.reserve(TERRAIN_GRID_SIZE * TERRAIN_GRID_SIZE);
}

Terrain::~Terrain() {
    terrainMesh = nullptr;
    heightData.clear();
}

bool Terrain::createFromHeightmap(const std::vector<float>& heightData,
                                 int32_t cellX, int32_t cellY) {
    if (heightData.size() != TERRAIN_GRID_SIZE * TERRAIN_GRID_SIZE) {
        return false;
    }

    this->heightData = heightData;
    this->cellX = cellX;
    this->cellY = cellY;
    this->basePosition = glm::vec3(cellX * CELL_SIZE, cellY * CELL_SIZE, 0.0f);

    normalizeHeights();
    return generateMeshFromHeightmap();
}

bool Terrain::createFlatTerrain(int32_t cellX, int32_t cellY) {
    // Create flat terrain with height = 0
    std::vector<float> flatHeights(TERRAIN_GRID_SIZE * TERRAIN_GRID_SIZE, 0.0f);
    return createFromHeightmap(flatHeights, cellX, cellY);
}

void Terrain::render(const glm::mat4& viewProj, ShaderProgram* shader) {
    if (!terrainMesh || !shader) {
        return;
    }

    // Create model matrix for this cell
    // Note: glm::mat4() creates identity matrix by default
    glm::mat4 modelMatrix = glm::mat4();
    modelMatrix = glm::translate(modelMatrix, basePosition);

    // Render terrain mesh
    // terrainMesh->render(*shader, modelMatrix);
}

float Terrain::getHeightAt(float localX, float localZ) const {
    clampLocalCoords(localX, localZ);

    // Convert local coordinates to grid indices
    float gridX = (localX / CELL_SIZE) * (TERRAIN_GRID_SIZE - 1);
    float gridZ = (localZ / CELL_SIZE) * (TERRAIN_GRID_SIZE - 1);

    int32_t gridXi = static_cast<int32_t>(gridX);
    int32_t gridZi = static_cast<int32_t>(gridZ);

    // Simple lookup (no interpolation)
    return getHeightAtIndex(gridXi, gridZi);
}

float Terrain::getInterpolatedHeight(float localX, float localZ) const {
    clampLocalCoords(localX, localZ);

    // Convert local coordinates to grid indices
    float gridX = (localX / CELL_SIZE) * (TERRAIN_GRID_SIZE - 1);
    float gridZ = (localZ / CELL_SIZE) * (TERRAIN_GRID_SIZE - 1);

    int32_t gridXi = static_cast<int32_t>(gridX);
    int32_t gridZi = static_cast<int32_t>(gridZ);

    // Get four corner heights
    // Note: Bilinear interpolation requires valid indices
    if (gridXi < 0) gridXi = 0;
    if (gridZi < 0) gridZi = 0;
    if (gridXi >= TERRAIN_GRID_SIZE - 1) gridXi = TERRAIN_GRID_SIZE - 2;
    if (gridZi >= TERRAIN_GRID_SIZE - 1) gridZi = TERRAIN_GRID_SIZE - 2;

    float h00 = getHeightAtIndex(gridXi, gridZi);
    float h10 = getHeightAtIndex(gridXi + 1, gridZi);
    float h01 = getHeightAtIndex(gridXi, gridZi + 1);
    float h11 = getHeightAtIndex(gridXi + 1, gridZi + 1);

    // Bilinear interpolation
    float fracX = gridX - gridXi;
    float fracZ = gridZ - gridZi;

    float h0 = h00 * (1.0f - fracX) + h10 * fracX;
    float h1 = h01 * (1.0f - fracX) + h11 * fracX;

    return h0 * (1.0f - fracZ) + h1 * fracZ;
}

glm::vec3 Terrain::getNormalAt(float localX, float localZ) const {
    // Simple normal calculation using finite differences
    float h0 = getHeightAt(localX - 1.0f, localZ);
    float h1 = getHeightAt(localX + 1.0f, localZ);
    float h2 = getHeightAt(localX, localZ - 1.0f);
    float h3 = getHeightAt(localX, localZ + 1.0f);

    glm::vec3 dx = glm::vec3(2.0f, h1 - h0, 0.0f);
    glm::vec3 dz = glm::vec3(0.0f, h3 - h2, 2.0f);

    // Manual cross product: cross(dx, dz)
    glm::vec3 normal(
        dx.y * dz.z - dx.z * dz.y,
        dx.z * dz.x - dx.x * dz.z,
        dx.x * dz.y - dx.y * dz.x
    );

    // Manual length calculation
    float len = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len > 0.0f) {
        return glm::vec3(normal.x / len, normal.y / len, normal.z / len);
    }
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

bool Terrain::isValidPosition(float localX, float localZ) const {
    return localX >= 0.0f && localX < CELL_SIZE &&
           localZ >= 0.0f && localZ < CELL_SIZE;
}

// ============================================================================
// Private Methods
// ============================================================================

bool Terrain::generateMeshFromHeightmap() {
    if (heightData.empty()) {
        return false;
    }

    // Create terrain mesh
    // This would be implemented with VAO/VBO generation
    // For now, create a simple mesh placeholder

    terrainMesh = std::make_shared<Mesh>();

    // Generate vertices from height data
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    for (int32_t z = 0; z < TERRAIN_GRID_SIZE; z++) {
        for (int32_t x = 0; x < TERRAIN_GRID_SIZE; x++) {
            float worldX = (x / static_cast<float>(TERRAIN_GRID_SIZE - 1)) * CELL_SIZE;
            float worldZ = (z / static_cast<float>(TERRAIN_GRID_SIZE - 1)) * CELL_SIZE;
            float height = heightData[z * TERRAIN_GRID_SIZE + x];

            vertices.push_back(glm::vec3(worldX, height, worldZ));
            normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));  // Simple upward normal
        }
    }

    // Generate indices for triangle strip
    for (int32_t z = 0; z < TERRAIN_GRID_SIZE - 1; z++) {
        for (int32_t x = 0; x < TERRAIN_GRID_SIZE - 1; x++) {
            uint32_t tl = z * TERRAIN_GRID_SIZE + x;
            uint32_t tr = tl + 1;
            uint32_t bl = (z + 1) * TERRAIN_GRID_SIZE + x;
            uint32_t br = bl + 1;

            // First triangle
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);

            // Second triangle
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    // Set mesh data (would call setVertices, setNormals, setIndices)
    // terrainMesh->setVertices(vertices);
    // terrainMesh->setNormals(normals);
    // terrainMesh->setIndices(indices);
    // terrainMesh->uploadToGPU();

    isInitialized = true;
    return true;
}

void Terrain::normalizeHeights() {
    // Optional: Normalize height values to a specific range
    // For flat terrain, heights are already 0.0
}

void Terrain::calculateVertexNormals() {
    // Calculate smooth vertex normals from heightmap
    // Implementation would iterate over vertices and calculate normals
}

void Terrain::clampLocalCoords(float& localX, float& localZ) const {
    if (localX < 0.0f) localX = 0.0f;
    if (localX >= CELL_SIZE) localX = CELL_SIZE - 0.001f;
    if (localZ < 0.0f) localZ = 0.0f;
    if (localZ >= CELL_SIZE) localZ = CELL_SIZE - 0.001f;
}

float Terrain::getHeightAtIndex(int32_t gridX, int32_t gridZ) const {
    // Clamp to grid bounds
    if (gridX < 0) gridX = 0;
    if (gridX >= TERRAIN_GRID_SIZE) gridX = TERRAIN_GRID_SIZE - 1;
    if (gridZ < 0) gridZ = 0;
    if (gridZ >= TERRAIN_GRID_SIZE) gridZ = TERRAIN_GRID_SIZE - 1;

    int32_t idx = gridZ * TERRAIN_GRID_SIZE + gridX;
    if (idx >= 0 && idx < static_cast<int32_t>(heightData.size())) {
        return heightData[idx];
    }

    return 0.0f;
}
