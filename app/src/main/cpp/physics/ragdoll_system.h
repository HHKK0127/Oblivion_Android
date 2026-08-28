#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <android/log.h>

#define OBLIVION_RAGDOLL_LOG_TAG "OblivionRagdoll"
#define OBLIVION_RAGDOLL_LOGI(...) __android_log_print(ANDROID_LOG_INFO, OBLIVION_RAGDOLL_LOG_TAG, __VA_ARGS__)
#define OBLIVION_RAGDOLL_LOGW(...) __android_log_print(ANDROID_LOG_WARN, OBLIVION_RAGDOLL_LOG_TAG, __VA_ARGS__)

namespace oblivion {

// Forward declaration only - do not include physics_manager.h.
class PhysicsManager;

// Joint category used to pick the correct Jolt constraint type when a bone
// link is created between two ragdoll bodies.
enum class RagdollJointType : uint8_t {
    HINGE = 0,        // elbow / knee - single axis rotation
    SWING_TWIST = 1,  // shoulder / hip - cone + twist rotation
    FIXED = 2         // spine segments, pelvis root
};

// Angle limits in degrees, matching Oblivion's original skeleton.nif
// bhkRagdollConstraint data as closely as is practical for Jolt.
struct RagdollJointLimits {
    float minAngleDeg = -45.0f;
    float maxAngleDeg = 45.0f;
    // Used only for SWING_TWIST joints.
    float swingLimitDeg = 60.0f;
    float twistLimitDeg = 30.0f;
};

// One bone entry in the ragdoll skeleton definition.
struct RagdollBoneDef {
    std::string boneName;        // e.g. "Bip01_L_UpperArm"
    std::string parentBoneName;  // empty for the root (Bip01_Pelvis)
    RagdollJointType jointType = RagdollJointType::SWING_TWIST;
    RagdollJointLimits limits;
    float capsuleRadius = 0.08f;
    float capsuleHalfHeight = 0.15f;
    float mass = 5.0f;
};

// Full ragdoll skeleton definition, built once and shared by every NPC that
// uses the default Oblivion humanoid skeleton.
struct RagdollDefinition {
    std::vector<RagdollBoneDef> bones;

    static RagdollDefinition createOblivionHumanoid() {
        RagdollDefinition def;
        auto add = [&](const char* name, const char* parent, RagdollJointType type,
                        float minDeg, float maxDeg, float radius, float halfHeight, float mass) {
            RagdollBoneDef b;
            b.boneName = name;
            b.parentBoneName = parent;
            b.jointType = type;
            b.limits.minAngleDeg = minDeg;
            b.limits.maxAngleDeg = maxDeg;
            b.limits.swingLimitDeg = maxDeg;
            b.limits.twistLimitDeg = maxDeg * 0.5f;
            b.capsuleRadius = radius;
            b.capsuleHalfHeight = halfHeight;
            b.mass = mass;
            def.bones.push_back(b);
        };

        // Root / torso chain (fixed - rigid spine like Oblivion's original havok rig)
        add("Bip01_Pelvis", "", RagdollJointType::FIXED, 0.0f, 0.0f, 0.14f, 0.10f, 12.0f);
        add("Bip01_Spine01", "Bip01_Pelvis", RagdollJointType::FIXED, -10.0f, 10.0f, 0.13f, 0.10f, 8.0f);
        add("Bip01_Spine02", "Bip01_Spine01", RagdollJointType::FIXED, -10.0f, 10.0f, 0.13f, 0.10f, 8.0f);
        add("Bip01_Spine03", "Bip01_Spine02", RagdollJointType::FIXED, -10.0f, 10.0f, 0.12f, 0.09f, 7.0f);
        add("Bip01_Head", "Bip01_Spine03", RagdollJointType::SWING_TWIST, -45.0f, 45.0f, 0.11f, 0.11f, 4.5f);

        // Arms
        add("Bip01_L_UpperArm", "Bip01_Spine03", RagdollJointType::SWING_TWIST, -90.0f, 90.0f, 0.06f, 0.14f, 2.5f);
        add("Bip01_L_Forearm", "Bip01_L_UpperArm", RagdollJointType::HINGE, 0.0f, 145.0f, 0.05f, 0.13f, 1.8f);
        add("Bip01_L_Hand", "Bip01_L_Forearm", RagdollJointType::SWING_TWIST, -30.0f, 30.0f, 0.045f, 0.06f, 0.6f);
        add("Bip01_R_UpperArm", "Bip01_Spine03", RagdollJointType::SWING_TWIST, -90.0f, 90.0f, 0.06f, 0.14f, 2.5f);
        add("Bip01_R_Forearm", "Bip01_R_UpperArm", RagdollJointType::HINGE, 0.0f, 145.0f, 0.05f, 0.13f, 1.8f);
        add("Bip01_R_Hand", "Bip01_R_Forearm", RagdollJointType::SWING_TWIST, -30.0f, 30.0f, 0.045f, 0.06f, 0.6f);

        // Legs
        add("Bip01_L_Thigh", "Bip01_Pelvis", RagdollJointType::SWING_TWIST, -100.0f, 45.0f, 0.09f, 0.20f, 6.0f);
        add("Bip01_L_Calf", "Bip01_L_Thigh", RagdollJointType::HINGE, -140.0f, 0.0f, 0.07f, 0.19f, 4.0f);
        add("Bip01_L_Foot", "Bip01_L_Calf", RagdollJointType::HINGE, -35.0f, 35.0f, 0.06f, 0.10f, 1.2f);
        add("Bip01_R_Thigh", "Bip01_Pelvis", RagdollJointType::SWING_TWIST, -100.0f, 45.0f, 0.09f, 0.20f, 6.0f);
        add("Bip01_R_Calf", "Bip01_R_Thigh", RagdollJointType::HINGE, -140.0f, 0.0f, 0.07f, 0.19f, 4.0f);
        add("Bip01_R_Foot", "Bip01_R_Calf", RagdollJointType::HINGE, -35.0f, 35.0f, 0.06f, 0.10f, 1.2f);

        return def;
    }
};

// Per-bone runtime state, tracking both the physical body and its blend
// weight against the animation pose.
struct RagdollBoneInstance {
    std::string boneName;
    JPH::BodyID bodyId;
    JPH::Constraint* constraint = nullptr;
    glm::mat4 animatedPose{};
    glm::mat4 physicsPose{};
    int parentIndex = -1;
};

enum class RagdollBlendState : uint8_t {
    INACTIVE = 0,
    BLENDING_TO_RAGDOLL,
    FULL_RAGDOLL,
    BLENDING_TO_ANIMATION
};

// One active ragdoll instance attached to an NPC.
class RagdollInstance {
public:
    uint32_t npcId = 0;
    RagdollBlendState state = RagdollBlendState::INACTIVE;
    float blendTimer = 0.0f;
    static constexpr float kBlendDuration = 0.3f; // seconds, per spec

    std::vector<RagdollBoneInstance> bones;

    // Returns blend weight [0..1] towards full ragdoll physics.
    float getRagdollWeight() const {
        switch (state) {
            case RagdollBlendState::INACTIVE:
                return 0.0f;
            case RagdollBlendState::BLENDING_TO_RAGDOLL:
                return std::min(std::max(blendTimer / kBlendDuration, 0.0f), 1.0f);
            case RagdollBlendState::FULL_RAGDOLL:
                return 1.0f;
            case RagdollBlendState::BLENDING_TO_ANIMATION:
                return std::min(std::max(1.0f - (blendTimer / kBlendDuration), 0.0f), 1.0f);
        }
        return 0.0f;
    }

    bool isActive() const { return state != RagdollBlendState::INACTIVE; }
};

// Manages ragdoll activation, blending, and pooling for NPC death/knockdown.
class RagdollSystem {
public:
    static RagdollSystem& getInstance() {
        static RagdollSystem instance;
        return instance;
    }

    void init(PhysicsManager* manager) {
        physicsManager = manager;
        skeletonDef = RagdollDefinition::createOblivionHumanoid();
        OBLIVION_RAGDOLL_LOGI("RagdollSystem initialized with %zu bones", skeletonDef.bones.size());
    }

    // Activates ragdoll physics for the given NPC, starting the animation ->
    // ragdoll blend. Reuses a pooled instance when available.
    RagdollInstance* activateRagdoll(uint32_t npcId, const glm::vec3& worldPosition) {
        RagdollInstance* instance = acquireFromPool();
        instance->npcId = npcId;
        instance->state = RagdollBlendState::BLENDING_TO_RAGDOLL;
        instance->blendTimer = 0.0f;

        if (instance->bones.empty()) {
            buildBonesForInstance(*instance, worldPosition);
        }

        activeInstances[npcId] = instance;
        OBLIVION_RAGDOLL_LOGI("Activated ragdoll for npc=%u", npcId);
        return instance;
    }

    // Starts the get-up transition (ragdoll -> animation) after the NPC
    // settles or an AI package requests it stand back up.
    void beginGetUp(uint32_t npcId) {
        auto it = activeInstances.find(npcId);
        if (it == activeInstances.end()) return;
        it->second->state = RagdollBlendState::BLENDING_TO_ANIMATION;
        it->second->blendTimer = 0.0f;
        OBLIVION_RAGDOLL_LOGI("Get-up started for npc=%u", npcId);
    }

    // Releases the ragdoll instance back into the pool for reuse on respawn.
    void deactivateRagdoll(uint32_t npcId) {
        auto it = activeInstances.find(npcId);
        if (it == activeInstances.end()) return;
        RagdollInstance* instance = it->second;
        instance->state = RagdollBlendState::INACTIVE;
        instance->npcId = 0;
        activeInstances.erase(it);
        pool.push_back(instance);
        OBLIVION_RAGDOLL_LOGI("Deactivated ragdoll, pool size=%zu", pool.size());
    }

    // Advances blend timers for all active ragdolls; call once per physics tick.
    void update(float deltaTime) {
        for (auto& [npcId, instance] : activeInstances) {
            if (instance->state == RagdollBlendState::BLENDING_TO_RAGDOLL ||
                instance->state == RagdollBlendState::BLENDING_TO_ANIMATION) {
                instance->blendTimer += deltaTime;
                if (instance->blendTimer >= RagdollInstance::kBlendDuration) {
                    if (instance->state == RagdollBlendState::BLENDING_TO_RAGDOLL) {
                        instance->state = RagdollBlendState::FULL_RAGDOLL;
                    } else {
                        // Get-up finished, return to full animation control.
                        deactivateRagdoll(npcId);
                    }
                }
            }
        }
    }

    // Blends a bone's animated pose with its physics pose using the
    // instance's current ragdoll weight. Called by the animation system
    // once per bone, per frame.
    static glm::mat4 blendBonePose(const RagdollBoneInstance& bone, float ragdollWeight) {
        // Simple linear blend of translation columns; a production
        // implementation should slerp the rotation component separately.
        glm::mat4 result{};
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result[col][row] = bone.animatedPose[col][row] * (1.0f - ragdollWeight) +
                                    bone.physicsPose[col][row] * ragdollWeight;
            }
        }
        return result;
    }

    RagdollInstance* getInstance(uint32_t npcId) {
        auto it = activeInstances.find(npcId);
        return it != activeInstances.end() ? it->second : nullptr;
    }

    const RagdollDefinition& getSkeletonDefinition() const { return skeletonDef; }

private:
    RagdollSystem() = default;

    RagdollInstance* acquireFromPool() {
        if (!pool.empty()) {
            RagdollInstance* instance = pool.back();
            pool.pop_back();
            return instance;
        }
        ownedInstances.push_back(std::make_unique<RagdollInstance>());
        return ownedInstances.back().get();
    }

    // Constructs bone body placeholders for a fresh (non-pooled) instance.
    // Actual JPH::Body creation/constraint wiring happens in the .cpp using
    // PhysicsManager's body interface; this header only defines layout.
    void buildBonesForInstance(RagdollInstance& instance, const glm::vec3& worldPosition) {
        instance.bones.reserve(skeletonDef.bones.size());
        std::unordered_map<std::string, int> nameToIndex;
        for (const auto& boneDef : skeletonDef.bones) {
            RagdollBoneInstance boneInstance;
            boneInstance.boneName = boneDef.boneName;
            boneInstance.animatedPose = glm::mat4();
            boneInstance.physicsPose = glm::mat4();
            if (!boneDef.parentBoneName.empty()) {
                auto parentIt = nameToIndex.find(boneDef.parentBoneName);
                boneInstance.parentIndex = parentIt != nameToIndex.end() ? parentIt->second : -1;
            }
            nameToIndex[boneDef.boneName] = static_cast<int>(instance.bones.size());
            instance.bones.push_back(boneInstance);
        }
        (void)worldPosition;
    }

    PhysicsManager* physicsManager = nullptr;
    RagdollDefinition skeletonDef;

    std::unordered_map<uint32_t, RagdollInstance*> activeInstances;
    std::vector<RagdollInstance*> pool;
    std::vector<std::unique_ptr<RagdollInstance>> ownedInstances;
};

} // namespace oblivion
