#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <android/log.h>
#include <glm/glm.hpp>

// Forward declarations
class WorldManager;
class Renderer;

// ============================================================================
// Distant LOD Manager - Phase 50
// Manages low-detail meshes for distant terrain rendering beyond active cells.
// Provides horizon ring, distance fade, and frustum culling for LOD meshes.
// ============================================================================

#define LOG_TAG_DLOD "DistantLodManager"
#define LOGD_DLOD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_DLOD, __VA_ARGS__)
#define LOGI_DLOD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_DLOD, __VA_ARGS__)
#define LOGW_DLOD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_DLOD, __VA_ARGS__)
#define LOGE_DLOD(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_DLOD, __VA_ARGS__)

// ============================================================================
// LOD Mesh Data - Vertex/index data for a single LOD mesh
// ============================================================================

struct LodMeshData {
    std::vector<float> vertices;        // x,y,z, u,v, r,g,b,a per vertex (stride=8)
    std::vector<uint16_t> indices;      // Triangle indices
    uint32_t textureId = 0;             // GL texture handle
    int lodLevel = 0;                   // 0=highest detail, increasing = lower detail
    int32_t cellX = 0;                  // World cell X coordinate
    int32_t cellY = 0;                  // World cell Y coordinate
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);  // Bounding sphere center
    float radius = 0.0f;               // Bounding sphere radius

    // GL buffer handles (managed by DistantLodManager)
    uint32_t vbo = 0;
    uint32_t ibo = 0;
    uint32_t vao = 0;
    bool gpuUploaded = false;
};

// ============================================================================
// Distant LOD Configuration
// ============================================================================

struct DistantLodConfig {
    float maxDistance = 2048.0f;         // Maximum render distance for LOD meshes
    float lodThresholdNear = 512.0f;    // Distance to switch from active to LOD
    float lodThresholdFar = 1024.0f;    // Distance to switch to lower LOD
    float horizonDistance = 4096.0f;    // Horizon ring radius
    float fadeStartDistance = 1024.0f;   // Distance where alpha fade begins
    float fadeEndDistance = 2048.0f;     // Distance where mesh is fully transparent
    float fogDensity = 0.0015f;         // Fog density factor
    float fogStart = 512.0f;            // Fog start distance
    int maxLodMeshes = 256;             // Maximum number of LOD meshes to track
    int downsampleFactor = 4;           // Heightmap downsample factor (65 -> ~16)
};

// ============================================================================
// Horizon Data - Far-distance mountain/hill mesh
// ============================================================================

struct HorizonData {
    std::vector<float> vertices;        // Ring vertex data
    std::vector<uint16_t> indices;      // Ring indices
    uint32_t vbo = 0;
    uint32_t ibo = 0;
    uint32_t vao = 0;
    bool gpuUploaded = false;
    float ringRadius = 4096.0f;
    float baseHeight = -50.0f;
    float peakHeight = 200.0f;
    int segmentCount = 64;
    int ringCount = 4;
};

// ============================================================================
// Frustum Plane (for culling)
// ============================================================================

struct DistantFrustumPlane {
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);
    float distance = 0.0f;
};

// ============================================================================
// DistantLodManager - Singleton
// ============================================================================

class DistantLodManager {
public:
    static DistantLodManager& instance();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool initialize(WorldManager* worldManager, void* landLoader = nullptr);
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // ========================================================================
    // LOD Mesh Registration
    // ========================================================================

    // Register a pre-built LOD mesh for a worldspace cell
    void registerLodMesh(const std::string& worldspaceId, const LodMeshData& data);

    // Generate LOD mesh from a LAND record heightmap
    // Returns the generated mesh data (caller can register it)
    LodMeshData generateLodFromLand(const std::vector<float>& heightData,
                                     int32_t cellX, int32_t cellY,
                                     int lodLevel,
                                     uint32_t textureId = 0);

    // Unregister LOD mesh by worldspace ID
    void unregisterLodMesh(const std::string& worldspaceId);

    // Clear all registered LOD meshes
    void clearAllMeshes();

    // ========================================================================
    // Visibility & Rendering
    // ========================================================================

    // Get visible LOD meshes based on camera position and view distance
    std::vector<const LodMeshData*> getVisibleLodMeshes(const glm::vec3& cameraPos,
                                                          float viewDistance) const;

    // Update frustum planes from view-projection matrix
    void updateFrustum(const glm::mat4& viewProj);

    // Frame update (distance calculations, fade updates)
    void update(float deltaTime);

    // Render all visible LOD meshes
    void render(Renderer* renderer, const glm::mat4& viewProj);

    // ========================================================================
    // Horizon Ring
    // ========================================================================

    // Generate and upload horizon ring mesh
    bool generateHorizonRing();

    // Render horizon ring
    void renderHorizonRing(const glm::mat4& viewProj, const glm::vec3& cameraPos);

    // ========================================================================
    // Configuration
    // ========================================================================

    void setConfig(const DistantLodConfig& cfg) { config = cfg; }
    const DistantLodConfig& getConfig() const { return config; }

    // ========================================================================
    // Statistics
    // ========================================================================

    struct DistantLodStats {
        uint32_t totalMeshes = 0;
        uint32_t visibleMeshes = 0;
        uint32_t culledMeshes = 0;
        uint32_t horizonSegments = 0;
        float nearestDistance = 0.0f;
        float farthestDistance = 0.0f;
    };

    const DistantLodStats& getStats() const { return stats; }
    void resetStats();

    // ========================================================================
    // Memory
    // ========================================================================

    size_t getGpuMemoryUsage() const;
    size_t getCpuMemoryUsage() const;

private:
    DistantLodManager() = default;
    ~DistantLodManager() = default;
    DistantLodManager(const DistantLodManager&) = delete;
    DistantLodManager& operator=(const DistantLodManager&) = delete;

    // ========================================================================
    // Member Variables
    // ========================================================================

    bool initialized_ = false;
    WorldManager* worldManager_ = nullptr;
    void* landLoader_ = nullptr;

    DistantLodConfig config;
    mutable DistantLodStats stats;

    // Registered LOD meshes keyed by worldspace ID
    std::unordered_map<std::string, LodMeshData> lodMeshes_;

    // Horizon ring
    HorizonData horizonData_;
    bool horizonGenerated_ = false;

    // Frustum planes (6: left, right, top, bottom, near, far)
    DistantFrustumPlane frustumPlanes_[6];
    bool frustumValid_ = false;

    // Shader program handle
    uint32_t shaderProgram_ = 0;
    bool shaderCompiled_ = false;

    // Uniform locations
    int locViewProj_ = -1;
    int locModel_ = -1;
    int locTexture_ = -1;
    int locFadeParams_ = -1;
    int locFogParams_ = -1;
    int locCameraPos_ = -1;
    int locAlpha_ = -1;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Frustum culling
    void extractFrustumPlanes(const glm::mat4& viewProj);
    void normalizeFrustumPlane(DistantFrustumPlane& plane);
    bool isSphereInFrustum(const glm::vec3& center, float radius) const;

    // GPU resource management
    void uploadMeshToGpu(LodMeshData& mesh);
    void uploadHorizonToGpu();
    void releaseMeshGpu(LodMeshData& mesh);
    void releaseHorizonGpu();

    // Shader
    bool compileShader();
    void deleteShader();

    // Downsample heightmap for LOD generation
    std::vector<float> downsampleHeightmap(const std::vector<float>& heightData,
                                            int originalSize,
                                            int targetSize) const;

    // Generate vertex color from height (for LOD tinting)
    void getTerrainColor(float height, float& r, float& g, float& b) const;

    // Calculate bounding sphere for mesh
    void calculateBoundingSphere(LodMeshData& mesh) const;
};
