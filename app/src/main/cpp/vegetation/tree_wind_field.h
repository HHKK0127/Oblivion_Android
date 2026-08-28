#pragma once

// ============================================================================
// WindField - Perlin noise-based spatial wind field for vegetation animation
// Phase 51: Provides spatially varying wind for natural tree sway
// ============================================================================

#include <glm/glm.hpp>
#include <cstdint>

namespace vegetation {

class WindField {
public:
    WindField();
    ~WindField();

    // Initialize with seed for reproducible noise
    void initialize(uint32_t seed = 42);

    // Sample wind at a world position at current time
    // Returns wind direction * strength as a vec3
    glm::vec3 sampleWind(const glm::vec3& worldPos, float time) const;

    // Set global wind parameters
    void setBaseDirection(const glm::vec3& dir) { baseDirection_ = glm::normalize(dir); }
    void setBaseStrength(float strength) { baseStrength_ = strength; }
    void setGustFrequency(float freq) { gustFrequency_ = freq; }
    void setGustStrength(float strength) { gustStrength_ = strength; }

    // Get current global wind direction (for passing to shaders)
    glm::vec3 getBaseDirection() const { return baseDirection_; }
    float getBaseStrength() const { return baseStrength_; }

    // Update wind state (call once per frame)
    void update(float deltaTime);

    // Get accumulated time
    float getTime() const { return time_; }

private:
    // Perlin noise implementation
    float perlinNoise2D(float x, float y) const;
    float perlinNoise3D(float x, float y, float z) const;
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y, float z) const;

    // Multi-octave fractal noise
    float fractalNoise(float x, float y, float z, int octaves) const;

    // Permutation table for noise
    int perm_[512];

    // Global wind state
    glm::vec3 baseDirection_ = glm::vec3(1.0f, 0.0f, 0.0f);
    float baseStrength_ = 0.3f;
    float gustFrequency_ = 0.5f;
    float gustStrength_ = 0.2f;
    float time_ = 0.0f;

    // Spatial scale for wind variation
    float spatialScale_ = 0.01f;

    // Time scale for wind variation
    float timeScale_ = 0.5f;

    uint32_t seed_ = 42;
};

} // namespace vegetation
