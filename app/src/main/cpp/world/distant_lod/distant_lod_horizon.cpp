#include "distant_lod_horizon.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// HorizonRing Implementation
// ============================================================================

HorizonRing::HorizonRing() {
    initDefaultPresets();
}

HorizonRing::~HorizonRing() {
    releaseGpu();
}

// ============================================================================
// Default Presets - 8 directional mountain profiles
// ============================================================================

void HorizonRing::initDefaultPresets() {
    // North - tall jagged peaks
    presets_[0].baseHeight = -60.0f;
    presets_[0].peakHeight = 250.0f;
    presets_[0].ridgeFrequency = 3.0f;
    presets_[0].ridgeAmplitude = 0.8f;
    presets_[0].noiseScale = 0.4f;
    presets_[0].baseColor = glm::vec3(0.22f, 0.20f, 0.16f);
    presets_[0].peakColor = glm::vec3(0.50f, 0.48f, 0.42f);

    // Northeast - rolling hills
    presets_[1].baseHeight = -40.0f;
    presets_[1].peakHeight = 150.0f;
    presets_[1].ridgeFrequency = 2.0f;
    presets_[1].ridgeAmplitude = 0.5f;
    presets_[1].noiseScale = 0.2f;
    presets_[1].baseColor = glm::vec3(0.24f, 0.22f, 0.17f);
    presets_[1].peakColor = glm::vec3(0.42f, 0.40f, 0.35f);

    // East - medium mountains
    presets_[2].baseHeight = -50.0f;
    presets_[2].peakHeight = 180.0f;
    presets_[2].ridgeFrequency = 2.5f;
    presets_[2].ridgeAmplitude = 0.65f;
    presets_[2].noiseScale = 0.3f;
    presets_[2].baseColor = glm::vec3(0.23f, 0.21f, 0.17f);
    presets_[2].peakColor = glm::vec3(0.46f, 0.44f, 0.38f);

    // Southeast - gentle slopes
    presets_[3].baseHeight = -35.0f;
    presets_[3].peakHeight = 120.0f;
    presets_[3].ridgeFrequency = 1.5f;
    presets_[3].ridgeAmplitude = 0.4f;
    presets_[3].noiseScale = 0.15f;
    presets_[3].baseColor = glm::vec3(0.25f, 0.23f, 0.18f);
    presets_[3].peakColor = glm::vec3(0.40f, 0.38f, 0.33f);

    // South - low rolling terrain
    presets_[4].baseHeight = -30.0f;
    presets_[4].peakHeight = 100.0f;
    presets_[4].ridgeFrequency = 1.8f;
    presets_[4].ridgeAmplitude = 0.35f;
    presets_[4].noiseScale = 0.2f;
    presets_[4].baseColor = glm::vec3(0.26f, 0.24f, 0.19f);
    presets_[4].peakColor = glm::vec3(0.38f, 0.36f, 0.31f);

    // Southwest - dramatic cliffs
    presets_[5].baseHeight = -70.0f;
    presets_[5].peakHeight = 220.0f;
    presets_[5].ridgeFrequency = 2.8f;
    presets_[5].ridgeAmplitude = 0.75f;
    presets_[5].noiseScale = 0.35f;
    presets_[5].baseColor = glm::vec3(0.21f, 0.19f, 0.15f);
    presets_[5].peakColor = glm::vec3(0.48f, 0.46f, 0.40f);

    // West - medium range
    presets_[6].baseHeight = -45.0f;
    presets_[6].peakHeight = 170.0f;
    presets_[6].ridgeFrequency = 2.2f;
    presets_[6].ridgeAmplitude = 0.6f;
    presets_[6].noiseScale = 0.25f;
    presets_[6].baseColor = glm::vec3(0.24f, 0.22f, 0.17f);
    presets_[6].peakColor = glm::vec3(0.44f, 0.42f, 0.36f);

    // Northwest - tall distant peaks
    presets_[7].baseHeight = -55.0f;
    presets_[7].peakHeight = 230.0f;
    presets_[7].ridgeFrequency = 3.2f;
    presets_[7].ridgeAmplitude = 0.85f;
    presets_[7].noiseScale = 0.45f;
    presets_[7].baseColor = glm::vec3(0.20f, 0.18f, 0.14f);
    presets_[7].peakColor = glm::vec3(0.52f, 0.50f, 0.44f);
}

// ============================================================================
// Pseudo-noise for terrain variation
// ============================================================================

float HorizonRing::pseudoNoise(float x, float y) const {
    // Simple hash-based noise (not Perlin, but sufficient for horizon variation)
    int n = static_cast<int>(x * 374761393.0f + y * 668265263.0f);
    n = (n << 13) ^ n;
    float val = static_cast<float>((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff);
    return val / 1073741823.0f;  // Normalize to [0, 1]
}

// ============================================================================
// Direction helpers
// ============================================================================

int HorizonRing::getDirectionIndex(float angle) const {
    // Map angle [0, 2*PI) to direction index [0, 7]
    float normalized = angle / (2.0f * static_cast<float>(M_PI));
    if (normalized < 0.0f) normalized += 1.0f;
    int idx = static_cast<int>(normalized * DIRECTION_COUNT) % DIRECTION_COUNT;
    return idx;
}

void HorizonRing::interpolatePresets(float angle, MountainPreset& result) const {
    float normalized = angle / (2.0f * static_cast<float>(M_PI));
    if (normalized < 0.0f) normalized += 1.0f;
    float scaled = normalized * DIRECTION_COUNT;
    int idx0 = static_cast<int>(scaled) % DIRECTION_COUNT;
    int idx1 = (idx0 + 1) % DIRECTION_COUNT;
    float t = scaled - static_cast<float>(static_cast<int>(scaled));

    const MountainPreset& p0 = presets_[idx0];
    const MountainPreset& p1 = presets_[idx1];

    result.baseHeight = p0.baseHeight + (p1.baseHeight - p0.baseHeight) * t;
    result.peakHeight = p0.peakHeight + (p1.peakHeight - p0.peakHeight) * t;
    result.ridgeFrequency = p0.ridgeFrequency + (p1.ridgeFrequency - p0.ridgeFrequency) * t;
    result.ridgeAmplitude = p0.ridgeAmplitude + (p1.ridgeAmplitude - p0.ridgeAmplitude) * t;
    result.noiseScale = p0.noiseScale + (p1.noiseScale - p0.noiseScale) * t;
    result.baseColor = p0.baseColor + (p1.baseColor - p0.baseColor) * t;
    result.peakColor = p0.peakColor + (p1.peakColor - p0.peakColor) * t;
}

// ============================================================================
// Mountain height at angle
// ============================================================================

float HorizonRing::getMountainHeight(float angle) const {
    MountainPreset preset;
    interpolatePresets(angle, preset);

    // Ridge component: sine wave with frequency
    float ridge = std::sin(angle * preset.ridgeFrequency) * preset.ridgeAmplitude;

    // Secondary ridge (different frequency for variation)
    float ridge2 = std::sin(angle * preset.ridgeFrequency * 2.3f + 1.5f) * preset.ridgeAmplitude * 0.4f;

    // Noise component
    float nx = std::cos(angle) * 10.0f;
    float ny = std::sin(angle) * 10.0f;
    float noise = pseudoNoise(nx, ny) * preset.noiseScale;

    // Combine: base + ridge + noise
    float heightRange = preset.peakHeight - preset.baseHeight;
    float combined = 0.5f + ridge * 0.3f + ridge2 * 0.15f + noise * 0.05f;
    combined = std::max(0.0f, std::min(1.0f, combined));

    return preset.baseHeight + heightRange * combined;
}

glm::vec3 HorizonRing::getMountainColor(float angle, float normalizedHeight) const {
    MountainPreset preset;
    interpolatePresets(angle, preset);
    return preset.baseColor + (preset.peakColor - preset.baseColor) * normalizedHeight;
}

// ============================================================================
// Generate mesh
// ============================================================================

bool HorizonRing::generate(float ringRadius, int segmentCount, int ringCount) {
    ringRadius_ = ringRadius;
    segmentCount_ = segmentCount;
    ringCount_ = ringCount;

    vertices_.clear();
    indices_.clear();

    if (segmentCount < 8 || ringCount < 2) {
        LOGE_HZN("Invalid parameters: segments=%d, rings=%d", segmentCount, ringCount);
        return false;
    }

    // Generate vertex grid: segmentCount columns x ringCount rows
    for (int ring = 0; ring < ringCount; ++ring) {
        float ringT = static_cast<float>(ring) / static_cast<float>(ringCount - 1);
        float height = baseHeight_ + (peakHeight_ - baseHeight_) * ringT;

        for (int seg = 0; seg <= segmentCount; ++seg) {
            float angle = static_cast<float>(seg) / static_cast<float>(segmentCount) * 2.0f * static_cast<float>(M_PI);

            // Mountain height modulation (only for upper rings)
            float mountainHeight = 0.0f;
            if (ring > 0) {
                mountainHeight = getMountainHeight(angle);
                // Scale mountain height by ring position
                height = baseHeight_ + (mountainHeight - baseHeight_) * ringT;
            }

            // Position on ring
            float x = std::cos(angle) * ringRadius;
            float z = std::sin(angle) * ringRadius;
            float y = height;

            // Color based on angle and height
            float normalizedHeight = static_cast<float>(ring) / static_cast<float>(ringCount - 1);
            glm::vec3 color = getMountainColor(angle, normalizedHeight);

            // Alpha: fully opaque at base, slightly transparent at peaks for blending
            float alpha = 1.0f;
            if (ring == ringCount - 1) {
                alpha = 0.7f;  // Top ring slightly transparent for sky blending
            }

            // Vertex: x, y, z, r, g, b, a
            vertices_.push_back(x);
            vertices_.push_back(y);
            vertices_.push_back(z);
            vertices_.push_back(color.x);
            vertices_.push_back(color.y);
            vertices_.push_back(color.z);
            vertices_.push_back(alpha);
        }
    }

    // Generate indices (triangle strip converted to indexed triangles)
    int vertsPerRing = segmentCount + 1;
    for (int ring = 0; ring < ringCount - 1; ++ring) {
        for (int seg = 0; seg < segmentCount; ++seg) {
            int topLeft = ring * vertsPerRing + seg;
            int topRight = topLeft + 1;
            int bottomLeft = (ring + 1) * vertsPerRing + seg;
            int bottomRight = bottomLeft + 1;

            // Two triangles per quad
            indices_.push_back(static_cast<uint16_t>(topLeft));
            indices_.push_back(static_cast<uint16_t>(bottomLeft));
            indices_.push_back(static_cast<uint16_t>(topRight));

            indices_.push_back(static_cast<uint16_t>(topRight));
            indices_.push_back(static_cast<uint16_t>(bottomLeft));
            indices_.push_back(static_cast<uint16_t>(bottomRight));
        }
    }

    generated_ = true;
    LOGI_HZN("Horizon ring generated: %d vertices, %d indices, radius=%.0f",
             getVertexCount(), getIndexCount(), ringRadius);
    return true;
}

// ============================================================================
// GPU Upload
// ============================================================================

bool HorizonRing::uploadToGpu() {
    if (!generated_) {
        LOGE_HZN("Cannot upload: mesh not generated");
        return false;
    }

    // Release previous GPU resources if any
    releaseGpu();

    // Create VAO
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // Create VBO
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
                 vertices_.data(), GL_STATIC_DRAW);

    // Position attribute (location 0): 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Color attribute (location 1): 4 floats
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create IBO
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices_.size() * sizeof(uint16_t)),
                 indices_.data(), GL_STATIC_DRAW);

    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    gpuUploaded_ = true;
    LOGI_HZN("Horizon ring uploaded to GPU: %zu bytes VBO, %zu bytes IBO",
             vertices_.size() * sizeof(float),
             indices_.size() * sizeof(uint16_t));
    return true;
}

void HorizonRing::releaseGpu() {
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (ibo_) { glDeleteBuffers(1, &ibo_); ibo_ = 0; }
    gpuUploaded_ = false;
}

// ============================================================================
// Render
// ============================================================================

void HorizonRing::render(const glm::mat4& viewProj, const glm::vec3& cameraPos,
                          uint32_t shaderProgram) {
    if (!gpuUploaded_ || !shaderProgram) return;

    glUseProgram(shaderProgram);

    // Set uniforms
    int locViewProj = glGetUniformLocation(shaderProgram, "uViewProj");
    int locCameraPos = glGetUniformLocation(shaderProgram, "uCameraPos");

    if (locViewProj >= 0) {
        glUniformMatrix4fv(locViewProj, 1, GL_FALSE,
                           reinterpret_cast<const float*>(&viewProj));
    }
    if (locCameraPos >= 0) {
        glUniform3f(locCameraPos, cameraPos.x, cameraPos.y, cameraPos.z);
    }

    // Enable blending for transparent top ring
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Draw
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()),
                   GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ============================================================================
// Preset access
// ============================================================================

void HorizonRing::setPreset(int direction, const MountainPreset& preset) {
    if (direction >= 0 && direction < DIRECTION_COUNT) {
        presets_[direction] = preset;
    }
}

const MountainPreset& HorizonRing::getPreset(int direction) const {
    static MountainPreset defaultPreset;
    if (direction >= 0 && direction < DIRECTION_COUNT) {
        return presets_[direction];
    }
    return defaultPreset;
}

size_t HorizonRing::getGpuMemoryBytes() const {
    if (!gpuUploaded_) return 0;
    return vertices_.size() * sizeof(float) + indices_.size() * sizeof(uint16_t);
}
