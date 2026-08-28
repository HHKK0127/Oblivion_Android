#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_OC "OcclusionCuller"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_OC(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_OC, __VA_ARGS__)
#else
#define LOGD_OC(...) do {} while(0)
#endif
#define LOGI_OC(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_OC, __VA_ARGS__)

// ============================================================================
// Occlusion Culling System
// Phase 55: Hierarchical Z-buffer with software rasterizer
// ============================================================================

namespace engine {

// Minimal vec3 for occlusion math
struct OccVec3 {
    float x, y, z;
};

struct OccMat4 {
    float m[16];
};

// Bounding sphere for occlusion test
struct OccBoundingSphere {
    OccVec3 center;
    float radius;
};

// Occlusion query result
enum class OcclusionResult : uint8_t {
    VISIBLE = 0,       // Object is visible, render it
    OCCLUDED = 1,      // Object is fully occluded, skip
    UNRESOLVED = 2     // Query pending, render conservatively
};

// Occluder mesh (simplified geometry for rasterization)
struct OccluderMesh {
    std::vector<OccVec3> vertices;
    std::vector<uint16_t> indices;
    OccVec3 center;
    float boundingRadius;
};

// ============================================================================
// HierarchicalZBuffer - multi-resolution depth buffer for occlusion testing
// ============================================================================

class HierarchicalZBuffer {
public:
    HierarchicalZBuffer() = default;

    void init(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;

        // Build mip chain (each level is half resolution)
        levels_.clear();
        uint32_t w = width;
        uint32_t h = height;
        while (w >= 2 && h >= 2) {
            MipLevel level;
            level.width = w;
            level.height = h;
            level.data.resize(w * h, 1.0f); // Initialize to far plane
            levels_.push_back(level);
            w /= 2;
            h /= 2;
        }

        LOGI_OC("HZB initialized: %ux%u, %zu mip levels",
                width, height, levels_.size());
    }

    void shutdown() {
        levels_.clear();
        width_ = 0;
        height_ = 0;
    }

    // Update from depth buffer (downsample)
    void updateFromDepth(const float* depthBuffer, uint32_t width, uint32_t height) {
        if (levels_.empty()) return;

        // Copy full-res depth to level 0
        MipLevel& level0 = levels_[0];
        if (level0.width == width && level0.height == height) {
            std::copy(depthBuffer, depthBuffer + width * height, level0.data.begin());
        }

        // Downsample: take max depth (nearest) of 2x2 blocks
        for (size_t i = 1; i < levels_.size(); i++) {
            const MipLevel& parent = levels_[i - 1];
            MipLevel& current = levels_[i];

            for (uint32_t y = 0; y < current.height; y++) {
                for (uint32_t x = 0; x < current.width; x++) {
                    uint32_t px = x * 2;
                    uint32_t py = y * 2;

                    float d00 = parent.data[py * parent.width + px];
                    float d10 = parent.data[py * parent.width + px + 1];
                    float d01 = parent.data[(py + 1) * parent.width + px];
                    float d11 = parent.data[(py + 1) * parent.width + px + 1];

                    // Max = nearest occluder depth
                    current.data[y * current.width + x] =
                        std::max(std::max(d00, d10), std::max(d01, d11));
                }
            }
        }
    }

    // Test a screen-space AABB against the HZB
    // Returns the maximum depth in the covered region
    float sampleMaxDepth(float minX, float minY, float maxX, float maxY,
                         uint32_t mipLevel = 0) const {
        if (mipLevel >= levels_.size()) return 1.0f;

        const MipLevel& level = levels_[mipLevel];

        // Clamp to screen bounds
        uint32_t x0 = static_cast<uint32_t>(std::max(0.0f, minX * level.width));
        uint32_t y0 = static_cast<uint32_t>(std::max(0.0f, minY * level.height));
        uint32_t x1 = static_cast<uint32_t>(std::min(static_cast<float>(level.width - 1),
                                                       maxX * level.width));
        uint32_t y1 = static_cast<uint32_t>(std::min(static_cast<float>(level.height - 1),
                                                       maxY * level.height));

        float maxDepth = 0.0f;
        for (uint32_t y = y0; y <= y1; y++) {
            for (uint32_t x = x0; x <= x1; x++) {
                float d = level.data[y * level.width + x];
                if (d > maxDepth) maxDepth = d;
            }
        }

        return maxDepth;
    }

    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    size_t getMipLevels() const { return levels_.size(); }

private:
    struct MipLevel {
        uint32_t width;
        uint32_t height;
        std::vector<float> data;
    };

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::vector<MipLevel> levels_;
};

// ============================================================================
// OcclusionCuller - main occlusion culling manager
// ============================================================================

class OcclusionCuller {
public:
    static OcclusionCuller& instance() {
        static OcclusionCuller inst;
        return inst;
    }

    void init(uint32_t screenWidth = 1920, uint32_t screenHeight = 1080) {
        std::lock_guard<std::mutex> lock(mutex_);
        hzb_.init(screenWidth / 2, screenHeight / 2); // Half-res HZB
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
        initialized_ = true;

        stats_ = {};
        LOGI_OC("OcclusionCuller initialized: %ux%u HZB", screenWidth / 2, screenHeight / 2);
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        hzb_.shutdown();
        occluders_.clear();
        initialized_ = false;
    }

    // --- Occluder registration ---

    // Register a static occluder (large meshes like buildings, terrain)
    uint32_t addOccluder(const OccluderMesh& mesh) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextOccluderId_++;
        occluders_[id] = mesh;
        return id;
    }

    void removeOccluder(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        occluders_.erase(id);
    }

    // --- Occlusion testing ---

    // Test a bounding sphere against the HZB
    OcclusionResult testSphere(const OccBoundingSphere& sphere,
                               const OccMat4& viewProj) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) return OcclusionResult::VISIBLE;

        stats_.totalTests++;

        // Project sphere to screen space
        float sx, sy, sr;
        if (!projectSphere(sphere, viewProj, sx, sy, sr)) {
            // Behind camera or degenerate
            stats_.visibleCount++;
            return OcclusionResult::VISIBLE;
        }

        // Compute screen-space AABB
        float minX = sx - sr;
        float minY = sy - sr;
        float maxX = sx + sr;
        float maxY = sy + sr;

        // Clamp to [0,1]
        minX = std::max(0.0f, std::min(1.0f, minX));
        minY = std::max(0.0f, std::min(1.0f, minY));
        maxX = std::max(0.0f, std::min(1.0f, maxX));
        maxY = std::max(0.0f, std::min(1.0f, maxY));

        // Choose mip level based on screen size
        float screenSize = (maxX - minX) * screenWidth_;
        uint32_t mipLevel = 0;
        if (screenSize < 4.0f) mipLevel = 3;
        else if (screenSize < 16.0f) mipLevel = 2;
        else if (screenSize < 64.0f) mipLevel = 1;

        // Sample HZB
        float hzbDepth = hzb_.sampleMaxDepth(minX, minY, maxX, maxY, mipLevel);

        // Sphere's nearest depth in screen space
        // Simplified: use center depth minus radius
        float sphereDepth = projectDepth(sphere.center, viewProj);
        float nearDepth = sphereDepth - sr * 0.5f; // Conservative

        if (nearDepth > hzbDepth) {
            // Sphere is behind all occluders
            stats_.occludedCount++;
            return OcclusionResult::OCCLUDED;
        }

        stats_.visibleCount++;
        return OcclusionResult::VISIBLE;
    }

    // Batch test for multiple objects
    std::vector<OcclusionResult> testBatch(
        const std::vector<OccBoundingSphere>& spheres,
        const OccMat4& viewProj
    ) {
        std::vector<OcclusionResult> results;
        results.reserve(spheres.size());
        for (const auto& sphere : spheres) {
            results.push_back(testSphere(sphere, viewProj));
        }
        return results;
    }

    // --- Frame management ---

    // Begin frame: update HZB from depth buffer
    void beginFrame(const float* depthBuffer, uint32_t width, uint32_t height) {
        std::lock_guard<std::mutex> lock(mutex_);
        hzb_.updateFromDepth(depthBuffer, width, height);
        stats_.totalTests = 0;
        stats_.visibleCount = 0;
        stats_.occludedCount = 0;
    }

    void endFrame() {
        // Could do temporal coherence optimization here
    }

    // --- Statistics ---

    struct OcclusionStats {
        uint32_t totalTests;
        uint32_t visibleCount;
        uint32_t occludedCount;
        float occlusionRate;
    };

    OcclusionStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        OcclusionStats s = stats_;
        if (s.totalTests > 0) {
            s.occlusionRate = static_cast<float>(s.occludedCount) / s.totalTests;
        }
        return s;
    }

private:
    OcclusionCuller() = default;

    bool initialized_ = false;
    uint32_t screenWidth_ = 0;
    uint32_t screenHeight_ = 0;
    uint32_t nextOccluderId_ = 1;

    HierarchicalZBuffer hzb_;
    std::unordered_map<uint32_t, OccluderMesh> occluders_;
    OcclusionStats stats_{};

    mutable std::mutex mutex_;

    // Project sphere center to screen space, compute screen-space radius
    bool projectSphere(const OccBoundingSphere& sphere, const OccMat4& vp,
                       float& sx, float& sy, float& sr) const {
        // Transform center
        const OccVec3& c = sphere.center;
        float w = vp.m[3] * c.x + vp.m[7] * c.y + vp.m[11] * c.z + vp.m[15];
        if (w <= 0.001f) return false; // Behind camera

        float cx = vp.m[0] * c.x + vp.m[4] * c.y + vp.m[8] * c.z + vp.m[12];
        float cy = vp.m[1] * c.x + vp.m[5] * c.y + vp.m[9] * c.z + vp.m[13];

        sx = (cx / w) * 0.5f + 0.5f;
        sy = (cy / w) * 0.5f + 0.5f;

        // Approximate screen-space radius
        sr = sphere.radius / w;

        return true;
    }

    float projectDepth(const OccVec3& p, const OccMat4& vp) const {
        float w = vp.m[3] * p.x + vp.m[7] * p.y + vp.m[11] * p.z + vp.m[15];
        if (w <= 0.001f) return 1.0f;
        float z = vp.m[2] * p.x + vp.m[6] * p.y + vp.m[10] * p.z + vp.m[14];
        return (z / w) * 0.5f + 0.5f;
    }
};

} // namespace engine
