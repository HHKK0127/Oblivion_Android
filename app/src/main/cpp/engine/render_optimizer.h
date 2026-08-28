#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <cstring>
#include <android/log.h>
#include <GLES3/gl3.h>

#define LOG_TAG_RENDEROPT "RenderOptimizer"
#define LOGD_RO(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_RENDEROPT, __VA_ARGS__)
#define LOGI_RO(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_RENDEROPT, __VA_ARGS__)
#define LOGW_RO(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_RENDEROPT, __VA_ARGS__)
#define LOGE_RO(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_RENDEROPT, __VA_ARGS__)

// ============================================================================
// Render Optimizer - Batch rendering and draw call optimization
// ============================================================================

class RenderOptimizer {
public:
    // ========================================================================
    // Render command for batching
    // ========================================================================

    struct RenderCommand {
        uint32_t vao = 0;
        uint32_t vbo = 0;
        uint32_t ebo = 0;
        uint32_t indexCount = 0;
        uint32_t materialId = 0;
        uint32_t shaderProgram = 0;
        uint32_t textureId = 0;
        float modelMatrix[16] = {};
        float position[3] = {};
        float boundingRadius = 1.0f;
        bool transparent = false;
        int32_t sortKey = 0;
    };

    // ========================================================================
    // Instance data for instanced rendering
    // ========================================================================

    struct InstanceData {
        float modelMatrix[16] = {};
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    // ========================================================================
    // Batch statistics
    // ========================================================================

    struct BatchStats {
        uint32_t totalCommands = 0;
        uint32_t batchedDrawCalls = 0;
        uint32_t savedDrawCalls = 0;
        uint32_t frustumCulled = 0;
        uint32_t instancesRendered = 0;
        float batchingRatio = 0.0f;
    };

    // ========================================================================
    // Frustum culling
    // ========================================================================

    struct FrustumPlane {
        float normal[3] = {};
        float distance = 0.0f;
    };

    RenderOptimizer();
    ~RenderOptimizer();

    bool initialize();
    void cleanup();

    // ========================================================================
    // Command submission
    // ========================================================================

    // Submit a render command for batching
    void submitCommand(const RenderCommand& cmd);

    // Begin a new frame (clear previous commands)
    void beginFrame();

    // Process all submitted commands (sort, batch, cull)
    void processCommands(const float* viewProjMatrix, const float* cameraPos);

    // Execute all batched draw calls
    void flush();

    // ========================================================================
    // Frustum culling
    // ========================================================================

    // Update frustum planes from view-projection matrix
    void updateFrustum(const float* viewProjMatrix);

    // Test if a sphere is visible
    bool isSphereVisible(const float* center, float radius) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const BatchStats& getStats() const { return stats_; }
    void resetStats();

    // ========================================================================
    // Configuration
    // ========================================================================

    void setMaxCommands(uint32_t max) { maxCommands_ = max; }
    void setFrustumCullingEnabled(bool enabled) { frustumCullingEnabled_ = enabled; }
    void setInstancingEnabled(bool enabled) { instancingEnabled_ = enabled; }

private:
    // ========================================================================
    // Batch group
    // ========================================================================

    struct BatchGroup {
        uint32_t materialId = 0;
        uint32_t shaderProgram = 0;
        uint32_t textureId = 0;
        uint32_t vao = 0;
        uint32_t vbo = 0;
        uint32_t ebo = 0;
        uint32_t indexCount = 0;
        std::vector<InstanceData> instances;
    };

    // ========================================================================
    // Member variables
    // ========================================================================

    std::vector<RenderCommand> commands_;
    std::vector<BatchGroup> batches_;
    FrustumPlane frustumPlanes_[6];
    BatchStats stats_;

    uint32_t maxCommands_ = 2048;
    bool frustumCullingEnabled_ = true;
    bool instancingEnabled_ = true;
    bool initialized_ = false;

    // Instance buffer for GPU instancing
    GLuint instanceVBO_ = 0;

    // ========================================================================
    // Private methods
    // ========================================================================

    // Sort commands by material/shader for batching
    void sortCommands();

    // Group sorted commands into batches
    void createBatches();

    // Execute a single batch
    void executeBatch(const BatchGroup& batch);

    // Normalize a frustum plane
    void normalizePlane(FrustumPlane& plane);

    // Generate sort key for a command
    int32_t generateSortKey(const RenderCommand& cmd) const;
};
