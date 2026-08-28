#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <android/log.h>
#include "physics_material.h"

#define OBLIVION_DESTROY_LOG_TAG "OblivionDestructionSystem"
#define OBLIVION_DESTROY_LOGI(...) __android_log_print(ANDROID_LOG_INFO, OBLIVION_DESTROY_LOG_TAG, __VA_ARGS__)
#define OBLIVION_DESTROY_LOGW(...) __android_log_print(ANDROID_LOG_WARN, OBLIVION_DESTROY_LOG_TAG, __VA_ARGS__)

namespace oblivion {

class PhysicsManager;

namespace detail {
// Lightweight random helpers, replacing glm::gtc/random.hpp which is not
// part of the bundled glm subset in this project.
inline float randomFloat(float minVal, float maxVal) {
    float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return minVal + t * (maxVal - minVal);
}

inline glm::vec3 randomOnSphere() {
    float theta = randomFloat(0.0f, 2.0f * 3.14159265358979323846f);
    float z = randomFloat(-1.0f, 1.0f);
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    return glm::vec3(r * std::cos(theta), r * std::sin(theta), z);
}
} // namespace detail

// Fracture pattern used when generating debris shards for a destroyed object.
enum class FracturePattern : uint8_t {
    VORONOI = 0,  // random cellular shards, good for crates/pottery
    RADIAL,       // explosion-centered wedges, good for barrels/explosive damage
    PLANAR        // clean slice, good for wood planks / fences
};

// One dropped item as part of loot on destruction (container contents).
struct LootDrop {
    std::string itemFormId;
    int count = 1;
    float dropChance = 1.0f;
};

// Definition of a breakable world object (crate, barrel, fence, urn, etc).
struct BreakableObjectDefinition {
    std::string objectId;
    MaterialType material = MaterialType::WOOD;
    float health = 20.0f;
    FracturePattern fracturePattern = FracturePattern::VORONOI;
    int debrisCount = 6;
    float debrisLifetimeSeconds = 10.0f;
    std::vector<LootDrop> lootTable;
    bool isTerrainProp = false; // fences/crates/barrels placed in the world
};

// A single spawned debris fragment.
struct DebrisFragment {
    JPH::BodyID bodyId;
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    float remainingLifetime = 0.0f;
    bool active = false;
    MaterialType material = MaterialType::WOOD;
};

// Runtime state for an object that has taken damage but not yet broken.
struct DestructibleInstance {
    uint32_t instanceId = 0;
    BreakableObjectDefinition definition;
    float currentHealth = 0.0f;
    JPH::BodyID bodyId;
    bool destroyed = false;
};

// Handles fracturing, debris pooling/lifetime, destruction sound hooks, and
// loot drops for breakable world objects (crates, barrels, fences, urns).
class DestructionSystem {
public:
    static DestructionSystem& getInstance() {
        static DestructionSystem instance;
        return instance;
    }

    void init(PhysicsManager* manager) {
        physicsManager = manager;
        debrisPool.reserve(kDebrisPoolCapacity);
        for (int i = 0; i < kDebrisPoolCapacity; ++i) {
            debrisPool.emplace_back();
        }
        OBLIVION_DESTROY_LOGI("DestructionSystem initialized with debris pool capacity=%d", kDebrisPoolCapacity);
    }

    // Registers a breakable instance in the world (e.g. when a crate/barrel
    // is streamed in). Returns an instance handle used for damage/destroy calls.
    uint32_t registerDestructible(const BreakableObjectDefinition& definition, JPH::BodyID bodyId) {
        uint32_t id = nextInstanceId++;
        DestructibleInstance instance;
        instance.instanceId = id;
        instance.definition = definition;
        instance.currentHealth = definition.health;
        instance.bodyId = bodyId;
        instances[id] = instance;
        return id;
    }

    void unregisterDestructible(uint32_t instanceId) {
        instances.erase(instanceId);
    }

    // Applies damage; triggers fracture + debris spawn once health <= 0.
    // Returns true if this call caused the object to be destroyed.
    bool applyDamage(uint32_t instanceId, float damage, const glm::vec3& hitPoint, const glm::vec3& hitDirection) {
        auto it = instances.find(instanceId);
        if (it == instances.end() || it->second.destroyed) return false;

        DestructibleInstance& instance = it->second;
        instance.currentHealth -= damage;
        if (instance.currentHealth <= 0.0f) {
            fracture(instance, hitPoint, hitDirection);
            return true;
        }
        return false;
    }

    // Immediately destroys an object regardless of health (e.g. scripted event).
    void forceDestroy(uint32_t instanceId, const glm::vec3& hitPoint, const glm::vec3& hitDirection) {
        auto it = instances.find(instanceId);
        if (it == instances.end() || it->second.destroyed) return;
        fracture(it->second, hitPoint, hitDirection);
    }

    // Advances debris lifetimes; call once per physics/game tick.
    void update(float deltaTime) {
        for (auto& fragment : debrisPool) {
            if (!fragment.active) continue;
            fragment.remainingLifetime -= deltaTime;
            if (fragment.remainingLifetime <= 0.0f) {
                recycleFragment(fragment);
            }
        }
    }

    size_t getActiveDebrisCount() const {
        size_t count = 0;
        for (const auto& f : debrisPool) if (f.active) ++count;
        return count;
    }

    // Debris pool capacity, exposed for diagnostics/tests.
    static constexpr int kDebrisPoolCapacity = 128;

private:
    DestructionSystem() = default;

    void fracture(DestructibleInstance& instance, const glm::vec3& hitPoint, const glm::vec3& hitDirection) {
        instance.destroyed = true;

        int shardCount = instance.definition.debrisCount;
        std::vector<glm::vec3> shardDirections = computeShardDirections(
            instance.definition.fracturePattern, shardCount, hitDirection);

        for (int i = 0; i < shardCount; ++i) {
            DebrisFragment* fragment = acquireFragment();
            if (!fragment) {
                OBLIVION_DESTROY_LOGW("Debris pool exhausted, dropping shard %d", i);
                break;
            }
            fragment->active = true;
            fragment->position = hitPoint;
            fragment->velocity = shardDirections[i] * kShardLaunchSpeed;
            fragment->remainingLifetime = instance.definition.debrisLifetimeSeconds;
            fragment->material = instance.definition.material;
            // Actual JPH body creation for the fragment shape happens in the
            // .cpp using PhysicsManager::createBox/createSphere equivalents.
        }

        playBreakSound(instance.definition.material, hitPoint);
        dropLoot(instance.definition.lootTable, hitPoint);

        OBLIVION_DESTROY_LOGI("Fractured instance=%u pattern=%d shards=%d",
                              instance.instanceId, static_cast<int>(instance.definition.fracturePattern), shardCount);
    }

    std::vector<glm::vec3> computeShardDirections(FracturePattern pattern, int count, const glm::vec3& hitDirection) {
        std::vector<glm::vec3> directions;
        directions.reserve(count);
        switch (pattern) {
            case FracturePattern::VORONOI:
                for (int i = 0; i < count; ++i) {
                    directions.push_back(detail::randomOnSphere());
                }
                break;
            case FracturePattern::RADIAL: {
                for (int i = 0; i < count; ++i) {
                    constexpr float kTwoPi = 6.28318530717958647692f;
                    float angle = (kTwoPi * static_cast<float>(i)) / static_cast<float>(count);
                    glm::vec3 dir(std::cos(angle), 0.3f, std::sin(angle));
                    directions.push_back(glm::normalize(dir));
                }
                break;
            }
            case FracturePattern::PLANAR: {
                glm::vec3 baseDir = glm::length(hitDirection) > 0.0001f ? glm::normalize(hitDirection)
                                                                          : glm::vec3(0.0f, 1.0f, 0.0f);
                for (int i = 0; i < count; ++i) {
                    glm::vec3 jitter = glm::vec3(detail::randomFloat(-0.2f, 0.2f), detail::randomFloat(-0.2f, 0.2f), detail::randomFloat(-0.2f, 0.2f));
                    directions.push_back(glm::normalize(baseDir + jitter));
                }
                break;
            }
        }
        return directions;
    }

    DebrisFragment* acquireFragment() {
        for (auto& fragment : debrisPool) {
            if (!fragment.active) {
                return &fragment;
            }
        }
        return nullptr;
    }

    void recycleFragment(DebrisFragment& fragment) {
        fragment.active = false;
        fragment.remainingLifetime = 0.0f;
        // Body removal from the physics world happens in the .cpp via
        // PhysicsManager::removeBody(fragment.bodyId).
    }

    void playBreakSound(MaterialType material, const glm::vec3& position) {
        const PhysicsMaterial* mat = PhysicsMaterialDatabase::getInstance().get(material);
        if (!mat) return;
        std::string sound = mat->getSound(SurfaceSoundEvent::BREAK_SOUND);
        if (sound.empty()) return;
        // Hook for the audio system; kept as a log line so this header has
        // no dependency on any audio manager include.
        OBLIVION_DESTROY_LOGI("Play break sound '%s' at (%.2f, %.2f, %.2f)",
                              sound.c_str(), position.x, position.y, position.z);
    }

    void dropLoot(const std::vector<LootDrop>& lootTable, const glm::vec3& position) {
        for (const auto& drop : lootTable) {
            if (drop.dropChance >= 1.0f || detail::randomFloat(0.0f, 1.0f) <= drop.dropChance) {
                OBLIVION_DESTROY_LOGI("Dropping loot '%s' x%d at (%.2f, %.2f, %.2f)",
                                      drop.itemFormId.c_str(), drop.count, position.x, position.y, position.z);
                // Actual world item spawn is delegated to the inventory/world system.
            }
        }
    }

    static constexpr float kShardLaunchSpeed = 4.0f;

    PhysicsManager* physicsManager = nullptr;
    uint32_t nextInstanceId = 1;
    std::unordered_map<uint32_t, DestructibleInstance> instances;
    std::vector<DebrisFragment> debrisPool;
};

} // namespace oblivion
