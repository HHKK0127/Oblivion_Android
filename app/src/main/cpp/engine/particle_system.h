#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <random>
#include <android/log.h>

#define LOG_TAG_PS "ParticleSystem"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_PS(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_PS, __VA_ARGS__)
#else
#define LOGD_PS(...) do {} while(0)
#endif
#define LOGI_PS(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_PS, __VA_ARGS__)

// ============================================================================
// Particle System
// Phase 56: GPU-friendly particle emitter with billboard rendering
// Oblivion effects: fire, smoke, magic, dust, rain, snow, sparks
// ============================================================================

namespace engine {

// Minimal vec3 for particles
struct PVec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    PVec3() = default;
    PVec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    PVec3 operator+(const PVec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    PVec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

// Single particle data (SoA-friendly)
struct Particle {
    PVec3 position;
    PVec3 velocity;
    PVec3 acceleration;
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 1.0f;
    float sizeEnd = 0.0f;
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float colorEnd[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    uint32_t textureIndex = 0;
    bool alive = false;
};

// Emitter shape types
enum class EmitterShape : uint8_t {
    POINT = 0,          // Emit from single point
    SPHERE,             // Emit from sphere surface
    BOX,                // Emit from box volume
    CONE,               // Emit in cone direction
    CYLINDER,           // Emit from cylinder
    MESH,               // Emit from mesh vertices
    LANDSCAPE           // Emit from terrain surface (Oblivion-specific)
};

// Blend mode for particles
enum class ParticleBlend : uint8_t {
    ALPHA = 0,
    ADDITIVE,
    MULTIPLY,
    PREMULTIPLIED
};

// Emitter configuration
struct EmitterConfig {
    std::string name;
    EmitterShape shape = EmitterShape::POINT;
    ParticleBlend blendMode = ParticleBlend::ALPHA;

    // Emission
    float emissionRate = 10.0f;         // Particles per second
    uint32_t maxParticles = 256;
    float burstCount = 0.0f;            // Burst emission (0 = continuous)
    float burstInterval = 0.0f;

    // Shape parameters
    PVec3 shapeSize = {1.0f, 1.0f, 1.0f};
    PVec3 emitDirection = {0.0f, 1.0f, 0.0f};
    float coneAngle = 45.0f;            // Degrees

    // Particle lifetime
    float lifeMin = 1.0f;
    float lifeMax = 3.0f;

    // Velocity
    float speedMin = 1.0f;
    float speedMax = 5.0f;

    // Size
    float sizeStartMin = 0.5f;
    float sizeStartMax = 1.0f;
    float sizeEndMin = 0.0f;
    float sizeEndMax = 0.1f;

    // Rotation
    float rotationMin = 0.0f;
    float rotationMax = 360.0f;
    float rotationSpeedMin = 0.0f;
    float rotationSpeedMax = 0.0f;

    // Color (start range)
    float colorStartMin[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float colorStartMax[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float colorEndMin[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    float colorEndMax[4] = {1.0f, 1.0f, 1.0f, 0.0f};

    // Physics
    PVec3 gravity = {0.0f, -9.8f, 0.0f};
    float drag = 0.0f;                  // Air resistance
    float turbulence = 0.0f;

    // Texture
    uint32_t textureId = 0;
    uint32_t subUVColumns = 1;          // Sub-UV atlas columns
    uint32_t subUVRows = 1;             // Sub-UV atlas rows
    bool randomStartFrame = false;

    // Looping
    bool looping = true;
    float duration = -1.0f;             // -1 = infinite
    float startDelay = 0.0f;
};

// ============================================================================
// ParticleEmitter - manages a single particle emitter
// ============================================================================

class ParticleEmitter {
public:
    explicit ParticleEmitter(const EmitterConfig& config)
        : config_(config) {
        particles_.resize(config.maxParticles);
        rng_.seed(std::random_device{}());
    }

    void update(float dt) {
        if (!active_) return;

        age_ += dt;

        // Start delay
        if (age_ < config_.startDelay) return;

        // Duration check
        if (config_.duration > 0.0f && age_ > config_.startDelay + config_.duration) {
            if (config_.looping) {
                age_ = config_.startDelay;
            } else {
                active_ = false;
                return;
            }
        }

        // Emit new particles
        emissionAccum_ += config_.emissionRate * dt;
        while (emissionAccum_ >= 1.0f && aliveCount_ < config_.maxParticles) {
            emitOne();
            emissionAccum_ -= 1.0f;
        }

        // Update particles
        for (auto& p : particles_) {
            if (!p.alive) continue;

            p.life -= dt;
            if (p.life <= 0.0f) {
                p.alive = false;
                aliveCount_--;
                continue;
            }

            // Physics
            p.velocity.x += (p.acceleration.x + config_.gravity.x) * dt;
            p.velocity.y += (p.acceleration.y + config_.gravity.y) * dt;
            p.velocity.z += (p.acceleration.z + config_.gravity.z) * dt;

            // Drag
            if (config_.drag > 0.0f) {
                float dragFactor = 1.0f - config_.drag * dt;
                dragFactor = std::max(0.0f, dragFactor);
                p.velocity.x *= dragFactor;
                p.velocity.y *= dragFactor;
                p.velocity.z *= dragFactor;
            }

            // Turbulence
            if (config_.turbulence > 0.0f) {
                p.velocity.x += randomRange(-config_.turbulence, config_.turbulence) * dt;
                p.velocity.y += randomRange(-config_.turbulence, config_.turbulence) * dt;
                p.velocity.z += randomRange(-config_.turbulence, config_.turbulence) * dt;
            }

            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.position.z += p.velocity.z * dt;

            // Rotation
            p.rotation += p.rotationSpeed * dt;

            // Interpolate size and color based on lifetime
            float t = 1.0f - (p.life / p.maxLife);
            float currentSize = p.size + (p.sizeEnd - p.size) * t;
            p.size = currentSize;

            for (int i = 0; i < 4; i++) {
                p.color[i] = p.color[i] + (p.colorEnd[i] - p.color[i]) * t;
            }
        }
    }

    void setActive(bool active) { active_ = active; }
    bool isActive() const { return active_; }
    uint32_t getAliveCount() const { return aliveCount_; }
    const EmitterConfig& getConfig() const { return config_; }

    // Get particle data for rendering (positions, sizes, colors)
    struct RenderData {
        std::vector<PVec3> positions;
        std::vector<float> sizes;
        std::vector<float> colors;      // RGBA interleaved
        std::vector<float> rotations;
        uint32_t count = 0;
    };

    RenderData getRenderData() const {
        RenderData rd;
        for (const auto& p : particles_) {
            if (!p.alive) continue;
            rd.positions.push_back(p.position);
            rd.sizes.push_back(p.size);
            rd.colors.push_back(p.color[0]);
            rd.colors.push_back(p.color[1]);
            rd.colors.push_back(p.color[2]);
            rd.colors.push_back(p.color[3]);
            rd.rotations.push_back(p.rotation);
        }
        rd.count = static_cast<uint32_t>(rd.positions.size());
        return rd;
    }

private:
    EmitterConfig config_;
    std::vector<Particle> particles_;
    bool active_ = true;
    float age_ = 0.0f;
    float emissionAccum_ = 0.0f;
    uint32_t aliveCount_ = 0;
    std::mt19937 rng_;

    void emitOne() {
        // Find dead particle
        for (auto& p : particles_) {
            if (p.alive) continue;

            p.alive = true;
            aliveCount_++;

            // Position based on shape
            p.position = generatePosition();

            // Velocity
            PVec3 dir = generateDirection();
            float speed = randomRange(config_.speedMin, config_.speedMax);
            p.velocity = dir * speed;

            // Life
            p.life = randomRange(config_.lifeMin, config_.lifeMax);
            p.maxLife = p.life;

            // Size
            p.size = randomRange(config_.sizeStartMin, config_.sizeStartMax);
            p.sizeEnd = randomRange(config_.sizeEndMin, config_.sizeEndMax);

            // Rotation
            p.rotation = randomRange(config_.rotationMin, config_.rotationMax);
            p.rotationSpeed = randomRange(config_.rotationSpeedMin, config_.rotationSpeedMax);

            // Color
            for (int i = 0; i < 4; i++) {
                p.color[i] = randomRange(config_.colorStartMin[i], config_.colorStartMax[i]);
                p.colorEnd[i] = randomRange(config_.colorEndMin[i], config_.colorEndMax[i]);
            }

            p.acceleration = {0.0f, 0.0f, 0.0f};
            return;
        }
    }

    PVec3 generatePosition() {
        switch (config_.shape) {
            case EmitterShape::POINT:
                return {0.0f, 0.0f, 0.0f};
            case EmitterShape::SPHERE: {
                float theta = randomRange(0.0f, 6.2831853f);
                float phi = randomRange(0.0f, 3.1415926f);
                float r = randomRange(0.0f, 1.0f);
                return {
                    std::sin(phi) * std::cos(theta) * r * config_.shapeSize.x,
                    std::cos(phi) * r * config_.shapeSize.y,
                    std::sin(phi) * std::sin(theta) * r * config_.shapeSize.z
                };
            }
            case EmitterShape::BOX:
                return {
                    randomRange(-config_.shapeSize.x, config_.shapeSize.x) * 0.5f,
                    randomRange(-config_.shapeSize.y, config_.shapeSize.y) * 0.5f,
                    randomRange(-config_.shapeSize.z, config_.shapeSize.z) * 0.5f
                };
            case EmitterShape::CONE: {
                float angle = randomRange(0.0f, config_.coneAngle) * 0.01745329f;
                float theta = randomRange(0.0f, 6.2831853f);
                float r = std::tan(angle) * randomRange(0.0f, config_.shapeSize.y);
                return {
                    std::cos(theta) * r,
                    0.0f,
                    std::sin(theta) * r
                };
            }
            case EmitterShape::CYLINDER: {
                float theta = randomRange(0.0f, 6.2831853f);
                float r = randomRange(0.0f, config_.shapeSize.x);
                return {
                    std::cos(theta) * r,
                    randomRange(0.0f, config_.shapeSize.y),
                    std::sin(theta) * r
                };
            }
            default:
                return {0.0f, 0.0f, 0.0f};
        }
    }

    PVec3 generateDirection() {
        if (config_.shape == EmitterShape::CONE) {
            // Main direction with spread
            PVec3 dir = config_.emitDirection;
            float spread = randomRange(0.0f, config_.coneAngle * 0.5f) * 0.01745329f;
            dir.x += randomRange(-spread, spread);
            dir.z += randomRange(-spread, spread);
            // Normalize
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (len > 0.001f) {
                dir.x /= len; dir.y /= len; dir.z /= len;
            }
            return dir;
        }
        // Random direction
        return {
            randomRange(-1.0f, 1.0f),
            randomRange(-1.0f, 1.0f),
            randomRange(-1.0f, 1.0f)
        };
    }

    float randomRange(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }
};

// ============================================================================
// ParticleSystem - manages all particle emitters
// ============================================================================

class ParticleSystem {
public:
    static ParticleSystem& instance() {
        static ParticleSystem inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);
        emitters_.clear();
        presets_.clear();
        registerPresets();
        initialized_ = true;
        LOGI_PS("ParticleSystem initialized with %zu presets", presets_.size());
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        emitters_.clear();
        presets_.clear();
        initialized_ = false;
    }

    void update(float dt) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, emitter] : emitters_) {
            emitter->update(dt);
        }
    }

    // Create emitter from config
    uint32_t createEmitter(const EmitterConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t id = nextId_++;
        emitters_[id] = std::make_unique<ParticleEmitter>(config);
        return id;
    }

    // Create emitter from preset
    uint32_t createFromPreset(const std::string& presetName) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = presets_.find(presetName);
        if (it == presets_.end()) {
            LOGI_PS("Unknown preset: %s", presetName.c_str());
            return 0;
        }
        uint32_t id = nextId_++;
        emitters_[id] = std::make_unique<ParticleEmitter>(it->second);
        return id;
    }

    void destroyEmitter(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        emitters_.erase(id);
    }

    void setEmitterActive(uint32_t id, bool active) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = emitters_.find(id);
        if (it != emitters_.end()) {
            it->second->setActive(active);
        }
    }

    // Get all render data for batched rendering
    std::vector<ParticleEmitter::RenderData> getAllRenderData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ParticleEmitter::RenderData> allData;
        for (const auto& [id, emitter] : emitters_) {
            if (emitter->isActive()) {
                allData.push_back(emitter->getRenderData());
            }
        }
        return allData;
    }

    uint32_t getEmitterCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<uint32_t>(emitters_.size());
    }

    uint32_t getTotalParticleCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t total = 0;
        for (const auto& [id, emitter] : emitters_) {
            total += emitter->getAliveCount();
        }
        return total;
    }

private:
    ParticleSystem() = default;

    bool initialized_ = false;
    uint32_t nextId_ = 1;
    std::unordered_map<uint32_t, std::unique_ptr<ParticleEmitter>> emitters_;
    std::unordered_map<std::string, EmitterConfig> presets_;
    mutable std::mutex mutex_;

    void registerPresets() {
        // Fire
        EmitterConfig fire;
        fire.name = "fire";
        fire.shape = EmitterShape::CONE;
        fire.blendMode = ParticleBlend::ADDITIVE;
        fire.emissionRate = 30.0f;
        fire.maxParticles = 128;
        fire.emitDirection = {0.0f, 1.0f, 0.0f};
        fire.coneAngle = 20.0f;
        fire.lifeMin = 0.5f; fire.lifeMax = 1.5f;
        fire.speedMin = 2.0f; fire.speedMax = 5.0f;
        fire.sizeStartMin = 0.3f; fire.sizeStartMax = 0.6f;
        fire.sizeEndMin = 0.0f; fire.sizeEndMax = 0.1f;
        fire.colorStartMin[0] = 1.0f; fire.colorStartMin[1] = 0.8f; fire.colorStartMin[2] = 0.2f; fire.colorStartMin[3] = 1.0f;
        fire.colorStartMax[0] = 1.0f; fire.colorStartMax[1] = 0.5f; fire.colorStartMax[2] = 0.0f; fire.colorStartMax[3] = 1.0f;
        fire.colorEndMin[0] = 0.3f; fire.colorEndMin[1] = 0.0f; fire.colorEndMin[2] = 0.0f; fire.colorEndMin[3] = 0.0f;
        fire.colorEndMax[0] = 0.5f; fire.colorEndMax[1] = 0.1f; fire.colorEndMax[2] = 0.0f; fire.colorEndMax[3] = 0.0f;
        fire.gravity = {0.0f, 2.0f, 0.0f}; // Updraft
        fire.drag = 1.0f;
        fire.looping = true;
        presets_["fire"] = fire;

        // Smoke
        EmitterConfig smoke;
        smoke.name = "smoke";
        smoke.shape = EmitterShape::CONE;
        smoke.blendMode = ParticleBlend::ALPHA;
        smoke.emissionRate = 8.0f;
        smoke.maxParticles = 64;
        smoke.emitDirection = {0.0f, 1.0f, 0.0f};
        smoke.coneAngle = 15.0f;
        smoke.lifeMin = 2.0f; smoke.lifeMax = 4.0f;
        smoke.speedMin = 0.5f; smoke.speedMax = 2.0f;
        smoke.sizeStartMin = 0.2f; smoke.sizeStartMax = 0.4f;
        smoke.sizeEndMin = 1.0f; smoke.sizeEndMax = 2.0f;
        smoke.colorStartMin[0] = 0.4f; smoke.colorStartMin[1] = 0.4f; smoke.colorStartMin[2] = 0.4f; smoke.colorStartMin[3] = 0.6f;
        smoke.colorEndMin[0] = 0.2f; smoke.colorEndMin[1] = 0.2f; smoke.colorEndMin[2] = 0.2f; smoke.colorEndMin[3] = 0.0f;
        smoke.gravity = {0.0f, 1.0f, 0.0f};
        smoke.drag = 2.0f;
        smoke.turbulence = 0.5f;
        smoke.looping = true;
        presets_["smoke"] = smoke;

        // Magic sparkles
        EmitterConfig magic;
        magic.name = "magic";
        magic.shape = EmitterShape::SPHERE;
        magic.blendMode = ParticleBlend::ADDITIVE;
        magic.emissionRate = 20.0f;
        magic.maxParticles = 100;
        magic.shapeSize = {0.5f, 0.5f, 0.5f};
        magic.lifeMin = 0.3f; magic.lifeMax = 1.0f;
        magic.speedMin = 1.0f; magic.speedMax = 3.0f;
        magic.sizeStartMin = 0.05f; magic.sizeStartMax = 0.15f;
        magic.sizeEndMin = 0.0f; magic.sizeEndMax = 0.02f;
        magic.colorStartMin[0] = 0.5f; magic.colorStartMin[1] = 0.5f; magic.colorStartMin[2] = 1.0f; magic.colorStartMin[3] = 1.0f;
        magic.colorStartMax[0] = 0.8f; magic.colorStartMax[1] = 0.8f; magic.colorStartMax[2] = 1.0f; magic.colorStartMax[3] = 1.0f;
        magic.colorEndMin[0] = 0.2f; magic.colorEndMin[1] = 0.2f; magic.colorEndMin[2] = 0.8f; magic.colorEndMin[3] = 0.0f;
        magic.gravity = {0.0f, 0.0f, 0.0f};
        magic.looping = true;
        presets_["magic"] = magic;

        // Rain
        EmitterConfig rain;
        rain.name = "rain";
        rain.shape = EmitterShape::BOX;
        rain.blendMode = ParticleBlend::ALPHA;
        rain.emissionRate = 200.0f;
        rain.maxParticles = 500;
        rain.shapeSize = {20.0f, 0.1f, 20.0f};
        rain.lifeMin = 1.0f; rain.lifeMax = 2.0f;
        rain.speedMin = 10.0f; rain.speedMax = 15.0f;
        rain.emitDirection = {0.0f, -1.0f, 0.0f};
        rain.sizeStartMin = 0.01f; rain.sizeStartMax = 0.02f;
        rain.sizeEndMin = 0.01f; rain.sizeEndMax = 0.02f;
        rain.colorStartMin[0] = 0.7f; rain.colorStartMin[1] = 0.8f; rain.colorStartMin[2] = 1.0f; rain.colorStartMin[3] = 0.4f;
        rain.gravity = {0.0f, -5.0f, 0.0f};
        rain.looping = true;
        presets_["rain"] = rain;

        // Snow
        EmitterConfig snow;
        snow.name = "snow";
        snow.shape = EmitterShape::BOX;
        snow.blendMode = ParticleBlend::ALPHA;
        snow.emissionRate = 50.0f;
        snow.maxParticles = 200;
        snow.shapeSize = {20.0f, 0.1f, 20.0f};
        snow.lifeMin = 3.0f; snow.lifeMax = 6.0f;
        snow.speedMin = 0.5f; snow.speedMax = 2.0f;
        snow.emitDirection = {0.0f, -1.0f, 0.0f};
        snow.sizeStartMin = 0.03f; snow.sizeStartMax = 0.08f;
        snow.sizeEndMin = 0.03f; snow.sizeEndMax = 0.08f;
        snow.colorStartMin[0] = 0.9f; snow.colorStartMin[1] = 0.95f; snow.colorStartMin[2] = 1.0f; snow.colorStartMin[3] = 0.8f;
        snow.gravity = {0.0f, -1.0f, 0.0f};
        snow.turbulence = 1.0f;
        snow.drag = 1.5f;
        snow.looping = true;
        presets_["snow"] = snow;

        // Dust
        EmitterConfig dust;
        dust.name = "dust";
        dust.shape = EmitterShape::SPHERE;
        dust.blendMode = ParticleBlend::ALPHA;
        dust.emissionRate = 5.0f;
        dust.maxParticles = 32;
        dust.shapeSize = {0.5f, 0.1f, 0.5f};
        dust.lifeMin = 1.0f; dust.lifeMax = 3.0f;
        dust.speedMin = 0.2f; dust.speedMax = 1.0f;
        dust.sizeStartMin = 0.1f; dust.sizeStartMax = 0.3f;
        dust.sizeEndMin = 0.3f; dust.sizeEndMax = 0.6f;
        dust.colorStartMin[0] = 0.6f; dust.colorStartMin[1] = 0.5f; dust.colorStartMin[2] = 0.3f; dust.colorStartMin[3] = 0.3f;
        dust.colorEndMin[0] = 0.4f; dust.colorEndMin[1] = 0.3f; dust.colorEndMin[2] = 0.2f; dust.colorEndMin[3] = 0.0f;
        dust.gravity = {0.0f, 0.2f, 0.0f};
        dust.turbulence = 0.3f;
        dust.looping = true;
        presets_["dust"] = dust;

        // Sparks
        EmitterConfig sparks;
        sparks.name = "sparks";
        sparks.shape = EmitterShape::POINT;
        sparks.blendMode = ParticleBlend::ADDITIVE;
        sparks.emissionRate = 15.0f;
        sparks.maxParticles = 50;
        sparks.lifeMin = 0.2f; sparks.lifeMax = 0.8f;
        sparks.speedMin = 3.0f; sparks.speedMax = 8.0f;
        sparks.sizeStartMin = 0.02f; sparks.sizeStartMax = 0.05f;
        sparks.sizeEndMin = 0.0f; sparks.sizeEndMax = 0.01f;
        sparks.colorStartMin[0] = 1.0f; sparks.colorStartMin[1] = 0.9f; sparks.colorStartMin[2] = 0.3f; sparks.colorStartMin[3] = 1.0f;
        sparks.colorEndMin[0] = 1.0f; sparks.colorEndMin[1] = 0.3f; sparks.colorEndMin[2] = 0.0f; sparks.colorEndMin[3] = 0.0f;
        sparks.gravity = {0.0f, -15.0f, 0.0f};
        sparks.looping = true;
        presets_["sparks"] = sparks;
    }
};

} // namespace engine
