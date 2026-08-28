#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_WR "WaterRenderer"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_WR(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_WR, __VA_ARGS__)
#else
#define LOGD_WR(...) do {} while(0)
#endif
#define LOGI_WR(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_WR, __VA_ARGS__)

// ============================================================================
// Water Renderer
// Phase 56: Oblivion-style water with reflections, refraction, waves
// ============================================================================

namespace engine {

// Water body types
enum class WaterType : uint8_t {
    RIVER = 0,      // Flowing water with current
    LAKE,           // Still water with gentle waves
    OCEAN,          // Large body with horizon
    SWAMP,          // Murky, dark water
    WATERFALL,      // Vertical flow
    OBLIVION_POOL   // Lava/chaos water in Oblivion realm
};

// Wave parameters
struct WaveParams {
    float amplitude = 0.3f;         // Wave height
    float frequency = 1.0f;         // Wave density
    float speed = 1.0f;             // Wave animation speed
    float steepness = 0.5f;         // Wave sharpness (0=sine, 1=sharp)
    float direction[2] = {1.0f, 0.0f}; // Wave direction
};

// Water material properties
struct WaterMaterial {
    float baseColor[3] = {0.1f, 0.3f, 0.5f};  // Deep water color
    float shallowColor[3] = {0.2f, 0.5f, 0.7f}; // Shallow water color
    float specularColor[3] = {1.0f, 1.0f, 1.0f};
    float transparency = 0.7f;
    float reflectivity = 0.5f;      // Fresnel reflectivity
    float refractionStrength = 0.1f;
    float specularPower = 64.0f;    // Shininess
    float foamThreshold = 0.8f;     // Wave height for foam
    float foamColor[3] = {0.9f, 0.95f, 1.0f};
    float causticsScale = 2.0f;     // Underwater light pattern scale
    float causticsSpeed = 0.5f;
};

// Water body configuration
struct WaterBodyConfig {
    std::string name;
    WaterType type = WaterType::LAKE;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float size[2] = {100.0f, 100.0f};  // Width, depth
    float waterLevel = 0.0f;
    float depth = 10.0f;                // Water depth
    WaterMaterial material;
    std::vector<WaveParams> waves;
    float flowSpeed = 0.0f;             // River current
    float flowDirection[2] = {1.0f, 0.0f};
    bool receiveShadows = true;
    bool castReflections = true;
    bool castCaustics = true;
};

// ============================================================================
// WaterBody - single water surface
// ============================================================================

class WaterBody {
public:
    explicit WaterBody(const WaterBodyConfig& config)
        : config_(config) {
        // Default waves if none specified
        if (config_.waves.empty()) {
            switch (config_.type) {
                case WaterType::RIVER:
                    config_.waves.push_back({0.15f, 2.0f, 1.5f, 0.3f, {1.0f, 0.0f}});
                    config_.waves.push_back({0.08f, 4.0f, 2.0f, 0.5f, {0.7f, 0.7f}});
                    break;
                case WaterType::LAKE:
                    config_.waves.push_back({0.1f, 1.0f, 0.5f, 0.2f, {1.0f, 0.0f}});
                    config_.waves.push_back({0.05f, 2.0f, 0.8f, 0.3f, {0.5f, 0.8f}});
                    break;
                case WaterType::OCEAN:
                    config_.waves.push_back({0.5f, 0.5f, 0.8f, 0.4f, {1.0f, 0.0f}});
                    config_.waves.push_back({0.3f, 1.0f, 1.2f, 0.5f, {0.7f, 0.3f}});
                    config_.waves.push_back({0.15f, 2.0f, 1.8f, 0.6f, {0.3f, 0.9f}});
                    config_.waves.push_back({0.08f, 4.0f, 2.5f, 0.7f, {-0.5f, 0.5f}});
                    break;
                case WaterType::SWAMP:
                    config_.waves.push_back({0.05f, 0.5f, 0.3f, 0.1f, {1.0f, 0.0f}});
                    break;
                case WaterType::WATERFALL:
                    config_.waves.push_back({0.2f, 3.0f, 3.0f, 0.8f, {0.0f, -1.0f}});
                    break;
                case WaterType::OBLIVION_POOL:
                    config_.waves.push_back({0.4f, 1.5f, 2.0f, 0.6f, {1.0f, 0.5f}});
                    config_.waves.push_back({0.2f, 3.0f, 3.0f, 0.7f, {-0.3f, 0.8f}});
                    break;
            }
        }
    }

    void update(float dt) {
        time_ += dt;
    }

    // Get wave height at world position (for CPU-side queries)
    float getWaveHeight(float x, float z) const {
        float height = 0.0f;
        for (const auto& wave : config_.waves) {
            float k = 2.0f * 3.1415926f / wave.frequency;
            float phase = k * (wave.direction[0] * x + wave.direction[1] * z) +
                         wave.speed * time_;
            // Gerstner wave approximation
            float s = std::sin(phase);
            height += wave.amplitude * s;
        }
        return height + config_.waterLevel;
    }

    // Get normal at world position
    void getNormal(float x, float z, float& nx, float& ny, float& nz) const {
        float eps = 0.1f;
        float hL = getWaveHeight(x - eps, z);
        float hR = getWaveHeight(x + eps, z);
        float hD = getWaveHeight(x, z - eps);
        float hU = getWaveHeight(x, z + eps);

        nx = hL - hR;
        ny = 2.0f * eps;
        nz = hD - hU;
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0.001f) {
            nx /= len; ny /= len; nz /= len;
        }
    }

    // Generate mesh vertices for water surface
    struct WaterVertex {
        float position[3];
        float normal[3];
        float uv[2];
    };

    std::vector<WaterVertex> generateMesh(uint32_t gridX, uint32_t gridZ) const {
        std::vector<WaterVertex> vertices;
        float halfW = config_.size[0] * 0.5f;
        float halfD = config_.size[1] * 0.5f;
        float stepX = config_.size[0] / gridX;
        float stepZ = config_.size[1] / gridZ;

        for (uint32_t z = 0; z <= gridZ; z++) {
            for (uint32_t x = 0; x <= gridX; x++) {
                float wx = config_.position[0] - halfW + x * stepX;
                float wz = config_.position[2] - halfD + z * stepZ;
                float wy = getWaveHeight(wx, wz);

                WaterVertex v;
                v.position[0] = wx;
                v.position[1] = wy;
                v.position[2] = wz;
                getNormal(wx, wz, v.normal[0], v.normal[1], v.normal[2]);
                v.uv[0] = static_cast<float>(x) / gridX;
                v.uv[1] = static_cast<float>(z) / gridZ;
                vertices.push_back(v);
            }
        }
        return vertices;
    }

    // Generate indices for triangle strip
    std::vector<uint16_t> generateIndices(uint32_t gridX, uint32_t gridZ) const {
        std::vector<uint16_t> indices;
        for (uint32_t z = 0; z < gridZ; z++) {
            for (uint32_t x = 0; x < gridX; x++) {
                uint16_t topLeft = z * (gridX + 1) + x;
                uint16_t topRight = topLeft + 1;
                uint16_t bottomLeft = (z + 1) * (gridX + 1) + x;
                uint16_t bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        return indices;
    }

    const WaterBodyConfig& getConfig() const { return config_; }
    float getTime() const { return time_; }

private:
    WaterBodyConfig config_;
    float time_ = 0.0f;
};

// ============================================================================
// WaterRenderer - manages all water bodies
// ============================================================================

class WaterRenderer {
public:
    static WaterRenderer& instance() {
        static WaterRenderer inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);
        waterBodies_.clear();
        initialized_ = true;
        LOGI_WR("WaterRenderer initialized");
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        waterBodies_.clear();
        initialized_ = false;
    }

    void update(float dt) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& body : waterBodies_) {
            body->update(dt);
        }
    }

    // Create water body
    uint32_t createWaterBody(const WaterBodyConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextId_++;
        waterBodies_.push_back(std::make_unique<WaterBody>(config));
        return id;
    }

    // Create from Oblivion LAND/WATER records
    uint32_t createFromESM(float x, float y, float z, float width, float depth,
                            WaterType type) {
        WaterBodyConfig config;
        config.name = "esm_water_" + std::to_string(nextId_);
        config.type = type;
        config.position[0] = x;
        config.position[1] = y;
        config.position[2] = z;
        config.size[0] = width;
        config.size[1] = depth;
        config.waterLevel = y;

        // Set material based on type
        switch (type) {
            case WaterType::RIVER:
                config.material.baseColor[0] = 0.1f;
                config.material.baseColor[1] = 0.3f;
                config.material.baseColor[2] = 0.5f;
                config.material.transparency = 0.6f;
                config.flowSpeed = 2.0f;
                break;
            case WaterType::LAKE:
                config.material.baseColor[0] = 0.05f;
                config.material.baseColor[1] = 0.2f;
                config.material.baseColor[2] = 0.4f;
                config.material.transparency = 0.7f;
                break;
            case WaterType::OCEAN:
                config.material.baseColor[0] = 0.02f;
                config.material.baseColor[1] = 0.1f;
                config.material.baseColor[2] = 0.3f;
                config.material.transparency = 0.5f;
                break;
            case WaterType::SWAMP:
                config.material.baseColor[0] = 0.15f;
                config.material.baseColor[1] = 0.2f;
                config.material.baseColor[2] = 0.1f;
                config.material.transparency = 0.3f;
                break;
            case WaterType::OBLIVION_POOL:
                config.material.baseColor[0] = 0.8f;
                config.material.baseColor[1] = 0.2f;
                config.material.baseColor[2] = 0.05f;
                config.material.transparency = 0.4f;
                config.material.shallowColor[0] = 1.0f;
                config.material.shallowColor[1] = 0.4f;
                config.material.shallowColor[2] = 0.1f;
                break;
            default:
                break;
        }

        return createWaterBody(config);
    }

    // Get all water bodies for rendering
    size_t getWaterBodyCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return waterBodies_.size();
    }

    WaterBody* getWaterBody(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index < waterBodies_.size()) {
            return waterBodies_[index].get();
        }
        return nullptr;
    }

    // Generate water shader source
    std::string generateWaterShader() const {
        std::string src;
        src += "#version 300 es\n";
        src += "precision highp float;\n";
        src += "in vec3 vWorldPos;\n";
        src += "in vec3 vNormal;\n";
        src += "in vec2 vUV;\n";
        src += "out vec4 fragColor;\n";
        src += "uniform sampler2D uReflectionTex;\n";
        src += "uniform sampler2D uRefractionTex;\n";
        src += "uniform sampler2D uDuDvMap;\n";
        src += "uniform sampler2D uNormalMap;\n";
        src += "uniform float uTime;\n";
        src += "uniform vec3 uCameraPos;\n";
        src += "uniform vec3 uSunDir;\n";
        src += "uniform vec3 uWaterColor;\n";
        src += "uniform float uTransparency;\n";
        src += "uniform float uReflectivity;\n";
        src += "\nvoid main() {\n";
        src += "    vec2 distortedUV = vUV + texture(uDuDvMap, vUV * 4.0 + uTime * 0.05).rg * 0.02;\n";
        src += "    vec3 normal = texture(uNormalMap, distortedUV).rgb * 2.0 - 1.0;\n";
        src += "    normal = normalize(mix(vNormal, normal, 0.5));\n";
        src += "    vec3 viewDir = normalize(uCameraPos - vWorldPos);\n";
        src += "    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);\n";
        src += "    fresnel = mix(0.1, 1.0, fresnel * uReflectivity);\n";
        src += "    vec2 reflUV = vec2(vUV.x, 1.0 - vUV.y) + normal.xz * 0.02;\n";
        src += "    vec4 reflColor = texture(uReflectionTex, reflUV);\n";
        src += "    vec4 refrColor = texture(uRefractionTex, vUV + normal.xz * 0.01);\n";
        src += "    vec3 color = mix(refrColor.rgb, reflColor.rgb, fresnel);\n";
        src += "    color = mix(color, uWaterColor, 0.3);\n";
        src += "    // Specular\n";
        src += "    vec3 halfDir = normalize(uSunDir + viewDir);\n";
        src += "    float spec = pow(max(dot(normal, halfDir), 0.0), 128.0);\n";
        src += "    color += vec3(1.0) * spec * 0.8;\n";
        src += "    fragColor = vec4(color, uTransparency);\n";
        src += "}\n";
        return src;
    }

private:
    WaterRenderer() = default;

    bool initialized_ = false;
    uint32_t nextId_ = 1;
    std::vector<std::unique_ptr<WaterBody>> waterBodies_;
    mutable std::mutex mutex_;
};

} // namespace engine
