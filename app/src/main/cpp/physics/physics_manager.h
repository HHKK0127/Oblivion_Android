#pragma once
#include "physics_types.h"
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <memory>
#include <unordered_map>

namespace oblivion {

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case static_cast<JPH::ObjectLayer>(PhysicsLayer::NON_MOVING):
                return inObject2 == static_cast<JPH::ObjectLayer>(PhysicsLayer::MOVING) ||
                       inObject2 == static_cast<JPH::ObjectLayer>(PhysicsLayer::TRIGGER);
            case static_cast<JPH::ObjectLayer>(PhysicsLayer::MOVING):
                return true;
            case static_cast<JPH::ObjectLayer>(PhysicsLayer::TRIGGER):
                return inObject2 == static_cast<JPH::ObjectLayer>(PhysicsLayer::MOVING);
            default:
                return false;
        }
    }
};

class BroadPhaseLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
public:
    BroadPhaseLayerInterfaceImpl() {
        mObjectToBroadPhase[static_cast<int>(PhysicsLayer::NON_MOVING)] = JPH::BroadPhaseLayer(0);
        mObjectToBroadPhase[static_cast<int>(PhysicsLayer::MOVING)]     = JPH::BroadPhaseLayer(1);
        mObjectToBroadPhase[static_cast<int>(PhysicsLayer::TRIGGER)]    = JPH::BroadPhaseLayer(2);
    }
    JPH::uint GetNumBroadPhaseLayers() const override { return 3; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        return mObjectToBroadPhase[inLayer];
    }
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const {
        switch (inLayer.GetValue()) {
            case 0: return "NON_MOVING";
            case 1: return "MOVING";
            case 2: return "TRIGGER";
            default: return "UNKNOWN";
        }
    }
private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[static_cast<int>(PhysicsLayer::NUM_LAYERS)];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case static_cast<JPH::ObjectLayer>(PhysicsLayer::NON_MOVING):
                return inLayer2 == JPH::BroadPhaseLayer(1) || inLayer2 == JPH::BroadPhaseLayer(2);
            case static_cast<JPH::ObjectLayer>(PhysicsLayer::MOVING):
                return true;
            case static_cast<JPH::ObjectLayer>(PhysicsLayer::TRIGGER):
                return inLayer2 == JPH::BroadPhaseLayer(1);
            default:
                return false;
        }
    }
};

class PhysicsManager {
public:
    static PhysicsManager& getInstance() {
        static PhysicsManager instance;
        return instance;
    }

    bool init();
    void update(float deltaTime);
    void shutdown();

    // Terrain: Generate HeightField from LANDRecord
    void createTerrainFromLand(const float* heightData, int size, float cellX, float cellY, float cellWorldSize);

    // Character (CharacterVirtual = Jolt recommended)
    JPH::CharacterVirtual* createCharacter(const glm::vec3& position, float height, float radius);
    void updateCharacter(JPH::CharacterVirtual* character, float deltaTime, const glm::vec3& input);
    glm::vec3 getCharacterPosition(JPH::CharacterVirtual* character) const;
    bool isCharacterGrounded(JPH::CharacterVirtual* character) const;
    void destroyCharacter(JPH::CharacterVirtual* character);

    // Dynamic objects
    JPH::BodyID createBox(const glm::vec3& pos, const glm::vec3& halfExtents, float mass);
    JPH::BodyID createSphere(const glm::vec3& pos, float radius, float mass);
    void setBodyPosition(JPH::BodyID bodyId, const glm::vec3& pos);
    glm::vec3 getBodyPosition(JPH::BodyID bodyId) const;
    void removeBody(JPH::BodyID bodyId);

    // レイキャスト
    bool raycast(const Ray& ray, RaycastHit& hit);

    JPH::PhysicsSystem* getSystem() { return physicsSystem; }

private:
    PhysicsManager() = default;
    ~PhysicsManager() { shutdown(); }

    JPH::PhysicsSystem* physicsSystem = nullptr;
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;

    BroadPhaseLayerInterfaceImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;

    float accumulator = 0.0f;
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
};

} // namespace oblivion
