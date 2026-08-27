#include "lod_system.h"
#include "../geometry/mesh.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// LODSystem Implementation
// ============================================================================

LODSystem::LODSystem()
    : frustumValid(false) {
}

LODSystem::~LODSystem() {
    cleanup();
}

bool LODSystem::initialize() {
    frustumValid = false;
    resetStats();

    LOGI_LOD("LODSystem initialized");
    return true;
}

void LODSystem::cleanup() {
    frustumValid = false;
    resetStats();

    LOGI_LOD("LODSystem cleaned up");
}

// ============================================================================
// LOD Configuration
// ============================================================================

void LODSystem::setConfig(const LODConfig& cfg) {
    config = cfg;

    LOGI_LOD("LOD config updated: High=%.0fm, Medium=%.0fm, Low=%.0fm, Billboard=%.0fm",
             config.highDistance, config.mediumDistance,
             config.lowDistance, config.billboardDistance);
}

// ============================================================================
// LOD Selection
// ============================================================================

LODSystem::LODLevel LODSystem::getLODLevel(float distance) const {
    if (distance <= config.highDistance) {
        return LODLevel::HIGH;
    } else if (distance <= config.mediumDistance) {
        return LODLevel::MEDIUM;
    } else if (distance <= config.lowDistance) {
        return LODLevel::LOW;
    } else {
        return LODLevel::BILLBOARD;
    }
}

LODSystem::LODLevel LODSystem::getLODLevel(const glm::vec3& objectPos,
                                              const glm::vec3& cameraPos) const {
    glm::vec3 diff = objectPos - cameraPos;
    float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    return getLODLevel(distance);
}

// ============================================================================
// Frustum Culling
// ============================================================================

void LODSystem::updateFrustum(const glm::mat4& viewProj) {
    extractFrustumPlanes(viewProj);
    frustumValid = true;
}

bool LODSystem::isPointInFrustum(const glm::vec3& point) const {
    if (!frustumValid) return true;  // If no frustum, assume visible

    for (int32_t i = 0; i < 6; ++i) {
        float dist = frustumPlanes[i].normal.x * point.x +
                     frustumPlanes[i].normal.y * point.y +
                     frustumPlanes[i].normal.z * point.z +
                     frustumPlanes[i].distance;

        if (dist < 0.0f) {
            return false;
        }
    }

    return true;
}

bool LODSystem::isSphereInFrustum(const glm::vec3& center, float radius) const {
    if (!frustumValid) return true;

    for (int32_t i = 0; i < 6; ++i) {
        float dist = frustumPlanes[i].normal.x * center.x +
                     frustumPlanes[i].normal.y * center.y +
                     frustumPlanes[i].normal.z * center.z +
                     frustumPlanes[i].distance;

        if (dist < -radius) {
            return false;
        }
    }

    return true;
}

bool LODSystem::isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const {
    if (!frustumValid) return true;

    for (int32_t i = 0; i < 6; ++i) {
        // Find the positive vertex (farthest along plane normal)
        glm::vec3 positiveVertex;
        positiveVertex.x = (frustumPlanes[i].normal.x >= 0.0f) ? max.x : min.x;
        positiveVertex.y = (frustumPlanes[i].normal.y >= 0.0f) ? max.y : min.y;
        positiveVertex.z = (frustumPlanes[i].normal.z >= 0.0f) ? max.z : min.z;

        float dist = frustumPlanes[i].normal.x * positiveVertex.x +
                     frustumPlanes[i].normal.y * positiveVertex.y +
                     frustumPlanes[i].normal.z * positiveVertex.z +
                     frustumPlanes[i].distance;

        if (dist < 0.0f) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Cell LOD Management
// ============================================================================

LODSystem::LODLevel LODSystem::getCellLODLevel(std::shared_ptr<Cell> cell,
                                                  const glm::vec3& cameraPos) const {
    if (!cell) return LODLevel::BILLBOARD;

    // Calculate cell center position
    float cellCenterX = static_cast<float>(cell->cellX) * CELL_SIZE + CELL_SIZE / 2.0f;
    float cellCenterZ = static_cast<float>(cell->cellY) * CELL_SIZE + CELL_SIZE / 2.0f;
    glm::vec3 cellCenter(cellCenterX, 0.0f, cellCenterZ);

    return getLODLevel(cellCenter, cameraPos);
}

bool LODSystem::shouldRenderCell(std::shared_ptr<Cell> cell,
                                    const glm::vec3& cameraPos,
                                    LODLevel minLOD) const {
    if (!cell) return false;

    // Check frustum culling
    float cellMinX = static_cast<float>(cell->cellX) * CELL_SIZE;
    float cellMinZ = static_cast<float>(cell->cellY) * CELL_SIZE;
    float cellMaxX = cellMinX + CELL_SIZE;
    float cellMaxZ = cellMinZ + CELL_SIZE;

    // Use a generous height range for frustum check
    glm::vec3 boxMin(cellMinX, -100.0f, cellMinZ);
    glm::vec3 boxMax(cellMaxX, 1000.0f, cellMaxZ);

    if (!isBoxInFrustum(boxMin, boxMax)) {
        stats.culledCount++;
        return false;
    }

    // Check LOD level
    LODLevel level = getCellLODLevel(cell, cameraPos);
    if (static_cast<uint8_t>(level) > static_cast<uint8_t>(minLOD)) {
        return false;
    }

    // Update stats
    switch (level) {
        case LODLevel::HIGH: stats.highDetailCount++; break;
        case LODLevel::MEDIUM: stats.mediumDetailCount++; break;
        case LODLevel::LOW: stats.lowDetailCount++; break;
        case LODLevel::BILLBOARD: stats.billboardCount++; break;
    }

    return true;
}

// ============================================================================
// Statistics
// ============================================================================

void LODSystem::resetStats() {
    stats = LODStats();
}

// ============================================================================
// Mesh LOD Generation
// ============================================================================

std::shared_ptr<Mesh> LODSystem::generateMediumLODMesh(const std::shared_ptr<Mesh>& highMesh) {
    if (!highMesh) return nullptr;

    // Medium LOD: reduce vertex count by ~50%
    // For terrain, skip every other vertex
    // For objects, use simplified mesh

    // For now, return the original mesh
    // TODO: Implement mesh simplification
    LOGD_LOD("Medium LOD mesh generation (placeholder)");
    return highMesh;
}

std::shared_ptr<Mesh> LODSystem::generateLowLODMesh(const std::shared_ptr<Mesh>& highMesh) {
    if (!highMesh) return nullptr;

    // Low LOD: reduce vertex count by ~75%
    // For terrain, use 33x33 grid instead of 65x65
    // For objects, use very simplified mesh

    // For now, return the original mesh
    // TODO: Implement mesh simplification
    LOGD_LOD("Low LOD mesh generation (placeholder)");
    return highMesh;
}

std::shared_ptr<Mesh> LODSystem::generateBillboardMesh(const glm::vec3& position,
                                                          float width, float height) {
    // Create a simple quad billboard
    auto mesh = std::make_shared<Mesh>();

    std::vector<Vertex> vertices(4);
    vertices[0].position = glm::vec3(position.x - width / 2, position.y, position.z);
    vertices[0].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[0].texCoord = glm::vec2(0.0f, 0.0f);
    vertices[0].color = glm::vec3(1.0f, 1.0f, 1.0f);

    vertices[1].position = glm::vec3(position.x + width / 2, position.y, position.z);
    vertices[1].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[1].texCoord = glm::vec2(1.0f, 0.0f);
    vertices[1].color = glm::vec3(1.0f, 1.0f, 1.0f);

    vertices[2].position = glm::vec3(position.x + width / 2, position.y + height, position.z);
    vertices[2].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[2].texCoord = glm::vec2(1.0f, 1.0f);
    vertices[2].color = glm::vec3(1.0f, 1.0f, 1.0f);

    vertices[3].position = glm::vec3(position.x - width / 2, position.y + height, position.z);
    vertices[3].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertices[3].texCoord = glm::vec2(0.0f, 1.0f);
    vertices[3].color = glm::vec3(1.0f, 1.0f, 1.0f);

    std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

    mesh->setVertices(vertices);
    mesh->setIndices(indices);

    return mesh;
}

// ============================================================================
// Private Methods
// ============================================================================

void LODSystem::normalizePlane(FrustumPlane& plane) {
    float length = std::sqrt(plane.normal.x * plane.normal.x +
                              plane.normal.y * plane.normal.y +
                              plane.normal.z * plane.normal.z);

    if (length > 0.0001f) {
        plane.normal /= length;
        plane.distance /= length;
    }
}

void LODSystem::extractFrustumPlanes(const glm::mat4& viewProj) {
    // Extract frustum planes from view-projection matrix
    // Using the Gribb-Hartmann method

    // Left plane
    frustumPlanes[0].normal.x = viewProj[0][3] + viewProj[0][0];
    frustumPlanes[0].normal.y = viewProj[1][3] + viewProj[1][0];
    frustumPlanes[0].normal.z = viewProj[2][3] + viewProj[2][0];
    frustumPlanes[0].distance = viewProj[3][3] + viewProj[3][0];

    // Right plane
    frustumPlanes[1].normal.x = viewProj[0][3] - viewProj[0][0];
    frustumPlanes[1].normal.y = viewProj[1][3] - viewProj[1][0];
    frustumPlanes[1].normal.z = viewProj[2][3] - viewProj[2][0];
    frustumPlanes[1].distance = viewProj[3][3] - viewProj[3][0];

    // Bottom plane
    frustumPlanes[2].normal.x = viewProj[0][3] + viewProj[0][1];
    frustumPlanes[2].normal.y = viewProj[1][3] + viewProj[1][1];
    frustumPlanes[2].normal.z = viewProj[2][3] + viewProj[2][1];
    frustumPlanes[2].distance = viewProj[3][3] + viewProj[3][1];

    // Top plane
    frustumPlanes[3].normal.x = viewProj[0][3] - viewProj[0][1];
    frustumPlanes[3].normal.y = viewProj[1][3] - viewProj[1][1];
    frustumPlanes[3].normal.z = viewProj[2][3] - viewProj[2][1];
    frustumPlanes[3].distance = viewProj[3][3] - viewProj[3][1];

    // Near plane
    frustumPlanes[4].normal.x = viewProj[0][3] + viewProj[0][2];
    frustumPlanes[4].normal.y = viewProj[1][3] + viewProj[1][2];
    frustumPlanes[4].normal.z = viewProj[2][3] + viewProj[2][2];
    frustumPlanes[4].distance = viewProj[3][3] + viewProj[3][2];

    // Far plane
    frustumPlanes[5].normal.x = viewProj[0][3] - viewProj[0][2];
    frustumPlanes[5].normal.y = viewProj[1][3] - viewProj[1][2];
    frustumPlanes[5].normal.z = viewProj[2][3] - viewProj[2][2];
    frustumPlanes[5].distance = viewProj[3][3] - viewProj[3][2];

    // Normalize all planes
    for (int32_t i = 0; i < 6; ++i) {
        normalizePlane(frustumPlanes[i]);
    }
}
