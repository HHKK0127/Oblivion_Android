#pragma once

#include "world_data.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <android/log.h>

// Forward declarations
class Mesh;

#define LOG_TAG_LOD "LODSystem"
#define LOGD_LOD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_LOD, __VA_ARGS__)
#define LOGI_LOD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_LOD, __VA_ARGS__)
#define LOGW_LOD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_LOD, __VA_ARGS__)
#define LOGE_LOD(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_LOD, __VA_ARGS__)

// ============================================================================
// LOD System - Distance-based Level of Detail management
// ============================================================================

class LODSystem {
public:
    LODSystem();
    ~LODSystem();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize();
    void cleanup();

    // ========================================================================
    // LOD Levels
    // ========================================================================

    enum class LODLevel : uint8_t {
        HIGH = 0,       // Full detail (0-100m)
        MEDIUM = 1,     // Reduced detail (100-300m)
        LOW = 2,        // Low detail (300-600m)
        BILLBOARD = 3   // Billboard/sprite (600m+)
    };

    // ========================================================================
    // LOD Configuration
    // ========================================================================

    struct LODConfig {
        float highDistance = 100.0f;      // Max distance for high LOD
        float mediumDistance = 300.0f;    // Max distance for medium LOD
        float lowDistance = 600.0f;       // Max distance for low LOD
        float billboardDistance = 1000.0f; // Max distance for billboard
    };

    // Set LOD configuration
    void setConfig(const LODConfig& config);

    // Get current configuration
    const LODConfig& getConfig() const { return config; }

    // ========================================================================
    // LOD Selection
    // ========================================================================

    // Get LOD level for a given distance
    LODLevel getLODLevel(float distance) const;

    // Get LOD level for an object at position relative to camera
    LODLevel getLODLevel(const glm::vec3& objectPos,
                          const glm::vec3& cameraPos) const;

    // ========================================================================
    // Frustum Culling
    // ========================================================================

    // Frustum plane structure
    struct FrustumPlane {
        glm::vec3 normal;
        float distance;
    };

    // Update frustum planes from view-projection matrix
    void updateFrustum(const glm::mat4& viewProj);

    // Check if a point is in the frustum
    bool isPointInFrustum(const glm::vec3& point) const;

    // Check if a sphere is in the frustum
    bool isSphereInFrustum(const glm::vec3& center, float radius) const;

    // Check if a bounding box is in the frustum
    bool isBoxInFrustum(const glm::vec3& min, const glm::vec3& max) const;

    // ========================================================================
    // Cell LOD Management
    // ========================================================================

    // Get LOD level for a cell based on distance from camera
    LODLevel getCellLODLevel(std::shared_ptr<Cell> cell,
                              const glm::vec3& cameraPos) const;

    // Check if cell should be rendered at given LOD
    bool shouldRenderCell(std::shared_ptr<Cell> cell,
                           const glm::vec3& cameraPos,
                           LODLevel minLOD = LODLevel::HIGH) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    struct LODStats {
        uint32_t highDetailCount = 0;
        uint32_t mediumDetailCount = 0;
        uint32_t lowDetailCount = 0;
        uint32_t billboardCount = 0;
        uint32_t culledCount = 0;
    };

    // Get current frame statistics
    const LODStats& getStats() const { return stats; }

    // Reset statistics
    void resetStats();

    // ========================================================================
    // Mesh LOD Generation
    // ========================================================================

    // Generate simplified mesh for medium LOD
    std::shared_ptr<Mesh> generateMediumLODMesh(const std::shared_ptr<Mesh>& highMesh);

    // Generate simplified mesh for low LOD
    std::shared_ptr<Mesh> generateLowLODMesh(const std::shared_ptr<Mesh>& highMesh);

    // Generate billboard mesh
    std::shared_ptr<Mesh> generateBillboardMesh(const glm::vec3& position,
                                                  float width, float height);

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    LODConfig config;
    mutable LODStats stats;

    // Frustum planes (6 planes: left, right, top, bottom, near, far)
    FrustumPlane frustumPlanes[6];
    bool frustumValid;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Normalize frustum plane
    void normalizePlane(FrustumPlane& plane);

    // Extract frustum planes from view-projection matrix
    void extractFrustumPlanes(const glm::mat4& viewProj);
};
