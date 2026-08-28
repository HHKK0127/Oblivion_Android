#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <android/log.h>

// ============================================================================
// Horizon Ring - Far-distance mountain silhouette
// Phase 50: Distant LOD System
// Generates a cylinder-like ring of low-poly mountain shapes around the player.
// 8 directional presets provide varied mountain profiles.
// ============================================================================

#define LOG_TAG_HORIZON "HorizonRing"
#define LOGD_HZN(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_HORIZON, __VA_ARGS__)
#define LOGI_HZN(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_HORIZON, __VA_ARGS__)
#define LOGW_HZN(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_HORIZON, __VA_ARGS__)
#define LOGE_HZN(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_HORIZON, __VA_ARGS__)

// ============================================================================
// Mountain Preset - Height profile for one direction
// ============================================================================

struct MountainPreset {
    float baseHeight = -50.0f;          // Base elevation (below horizon)
    float peakHeight = 200.0f;          // Maximum peak elevation
    float ridgeFrequency = 2.0f;        // How many ridges in this direction
    float ridgeAmplitude = 0.7f;        // Ridge height variation (0-1)
    float noiseScale = 0.3f;            // Additional noise variation
    glm::vec3 baseColor = glm::vec3(0.25f, 0.22f, 0.18f);  // Dark earth
    glm::vec3 peakColor = glm::vec3(0.45f, 0.42f, 0.38f);  // Light rock
};

// ============================================================================
// HorizonRing - Generates and manages the horizon mountain ring
// ============================================================================

class HorizonRing {
public:
    HorizonRing();
    ~HorizonRing();

    // ========================================================================
    // Generation
    // ========================================================================

    // Generate the full horizon ring mesh
    // ringRadius: distance from center to ring
    // segmentCount: number of segments around the ring (higher = smoother)
    // ringCount: number of vertical rings (higher = more detail)
    bool generate(float ringRadius = 4096.0f, int segmentCount = 64, int ringCount = 4);

    // ========================================================================
    // GPU Upload
    // ========================================================================

    // Upload mesh data to GPU (creates VBO/IBO/VAO)
    bool uploadToGpu();

    // Release GPU resources
    void releaseGpu();

    // ========================================================================
    // Rendering
    // ========================================================================

    // Render the horizon ring
    // viewProj: view-projection matrix
    // cameraPos: current camera position (ring follows camera XZ)
    void render(const glm::mat4& viewProj, const glm::vec3& cameraPos,
                uint32_t shaderProgram);

    // ========================================================================
    // Configuration
    // ========================================================================

    // Set mountain preset for a specific direction (0-7, clockwise from North)
    void setPreset(int direction, const MountainPreset& preset);

    // Get preset for direction
    const MountainPreset& getPreset(int direction) const;

    // Set global ring parameters
    void setRingRadius(float radius) { ringRadius_ = radius; }
    void setBaseHeight(float height) { baseHeight_ = height; }
    void setPeakHeight(float height) { peakHeight_ = height; }

    // ========================================================================
    // Queries
    // ========================================================================

    bool isGenerated() const { return generated_; }
    bool isUploaded() const { return gpuUploaded_; }
    int getVertexCount() const { return static_cast<int>(vertices_.size()) / 7; }
    int getIndexCount() const { return static_cast<int>(indices_.size()); }
    size_t getGpuMemoryBytes() const;

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    bool generated_ = false;
    bool gpuUploaded_ = false;

    // Mesh data: x,y,z, r,g,b,a per vertex (stride=7)
    std::vector<float> vertices_;
    std::vector<uint16_t> indices_;

    // GPU handles
    uint32_t vbo_ = 0;
    uint32_t ibo_ = 0;
    uint32_t vao_ = 0;

    // Ring parameters
    float ringRadius_ = 4096.0f;
    float baseHeight_ = -50.0f;
    float peakHeight_ = 200.0f;
    int segmentCount_ = 64;
    int ringCount_ = 4;

    // 8 directional mountain presets (N, NE, E, SE, S, SW, W, NW)
    static constexpr int DIRECTION_COUNT = 8;
    MountainPreset presets_[DIRECTION_COUNT];

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Initialize default mountain presets
    void initDefaultPresets();

    // Get interpolated mountain height at angle
    float getMountainHeight(float angle) const;

    // Get interpolated color at angle and height
    glm::vec3 getMountainColor(float angle, float normalizedHeight) const;

    // Simple pseudo-noise for terrain variation
    float pseudoNoise(float x, float y) const;

    // Get direction index from angle (0-7)
    int getDirectionIndex(float angle) const;

    // Interpolate between two presets based on angle
    void interpolatePresets(float angle, MountainPreset& result) const;
};
