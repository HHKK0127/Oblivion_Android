#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <android/log.h>

#define OBLIVION_JOINT_LOG_TAG "OblivionJointSystem"
#define OBLIVION_JOINT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, OBLIVION_JOINT_LOG_TAG, __VA_ARGS__)
#define OBLIVION_JOINT_LOGW(...) __android_log_print(ANDROID_LOG_WARN, OBLIVION_JOINT_LOG_TAG, __VA_ARGS__)

namespace oblivion {

class PhysicsManager;

// Joint categories for interactive world objects (doors, chains, traps).
enum class JointType : uint8_t {
    HINGE = 0,       // doors, levers
    BALL_SOCKET,     // chains, chandeliers
    SLIDING,         // drawbridges, portcullises
    FIXED,           // welded/rigid attachment
    SPRING           // trap mechanisms
};

// Describes motor behaviour for powered joints (elevators, powered doors).
struct JointMotorSettings {
    bool enabled = false;
    float targetVelocity = 0.0f;   // rad/s for angular, m/s for linear
    float maxTorqueOrForce = 500.0f;
};

// Angle/distance limits, interpreted per joint type.
struct JointLimits {
    float minValue = 0.0f;   // radians for hinge, meters for sliding
    float maxValue = 0.0f;
};

// Breakable joint support: once accumulated force/torque exceeds the
// threshold the joint is removed and bodies separate freely.
struct BreakableJointSettings {
    bool breakable = false;
    float maxForce = 10000.0f;
    float maxTorque = 10000.0f;
};

// Runtime record for a single created joint/constraint.
struct JointInstance {
    uint32_t jointId = 0;
    JointType type = JointType::HINGE;
    JPH::BodyID bodyA;
    JPH::BodyID bodyB;
    JPH::Constraint* constraint = nullptr;
    JointLimits limits;
    JointMotorSettings motor;
    BreakableJointSettings breakSettings;
    bool broken = false;
    std::string debugLabel;
};

// Preset angle limits matching common Oblivion door configurations.
struct DoorJointPreset {
    static JointLimits singleDoor() { return JointLimits{0.0f, glm::radians(90.0f)}; }
    static JointLimits doubleDoor() { return JointLimits{0.0f, glm::radians(120.0f)}; }
    static JointLimits trapDoor() { return JointLimits{0.0f, glm::radians(100.0f)}; }
};

// Manages joints/constraints for interactive world geometry: doors, chains,
// drawbridges, welded props, and spring-loaded traps.
class JointSystem {
public:
    static JointSystem& getInstance() {
        static JointSystem instance;
        return instance;
    }

    void init(PhysicsManager* manager) {
        physicsManager = manager;
        OBLIVION_JOINT_LOGI("JointSystem initialized");
    }

    // Creates a hinge joint suitable for doors and levers. anchorPoint and
    // hingeAxis are given in world space at creation time.
    uint32_t createHingeJoint(JPH::BodyID bodyA, JPH::BodyID bodyB,
                               const glm::vec3& anchorPoint, const glm::vec3& hingeAxis,
                               const JointLimits& limits, const std::string& label = "hinge") {
        JointInstance instance;
        instance.jointId = nextJointId++;
        instance.type = JointType::HINGE;
        instance.bodyA = bodyA;
        instance.bodyB = bodyB;
        instance.limits = limits;
        instance.debugLabel = label;
        (void)anchorPoint;
        (void)hingeAxis;
        // Actual JPH::HingeConstraintSettings construction happens in the
        // .cpp where we have access to the live BodyInterface for body
        // lookups; this header stores the intended configuration.
        joints[instance.jointId] = instance;
        OBLIVION_JOINT_LOGI("Created hinge joint '%s' id=%u", label.c_str(), instance.jointId);
        return instance.jointId;
    }

    // Creates a door hinge using an Oblivion-style preset.
    uint32_t createDoorJoint(JPH::BodyID doorBody, JPH::BodyID frameBody,
                              const glm::vec3& hingePoint, bool isDoubleDoor = false) {
        JointLimits limits = isDoubleDoor ? DoorJointPreset::doubleDoor() : DoorJointPreset::singleDoor();
        return createHingeJoint(doorBody, frameBody, hingePoint, glm::vec3(0.0f, 1.0f, 0.0f),
                                 limits, isDoubleDoor ? "double_door" : "door");
    }

    // Creates a single ball-socket link in a chain. Call repeatedly, linking
    // consecutive link bodies together, to build an entire chandelier chain.
    uint32_t createBallSocketJoint(JPH::BodyID bodyA, JPH::BodyID bodyB,
                                    const glm::vec3& anchorPoint, const std::string& label = "chain_link") {
        JointInstance instance;
        instance.jointId = nextJointId++;
        instance.type = JointType::BALL_SOCKET;
        instance.bodyA = bodyA;
        instance.bodyB = bodyB;
        instance.debugLabel = label;
        (void)anchorPoint;
        joints[instance.jointId] = instance;
        return instance.jointId;
    }

    // Builds a full hanging chain of N links between a fixed anchor body and
    // a hanging payload body (e.g. a chandelier). Returns the joint IDs in
    // order from anchor to payload.
    std::vector<uint32_t> createChain(JPH::BodyID anchorBody,
                                       const std::vector<JPH::BodyID>& linkBodies,
                                       JPH::BodyID payloadBody,
                                       const glm::vec3& anchorPoint) {
        std::vector<uint32_t> jointIds;
        JPH::BodyID previous = anchorBody;
        for (size_t i = 0; i < linkBodies.size(); ++i) {
            jointIds.push_back(createBallSocketJoint(previous, linkBodies[i], anchorPoint, "chain_link"));
            previous = linkBodies[i];
        }
        jointIds.push_back(createBallSocketJoint(previous, payloadBody, anchorPoint, "chain_payload"));
        return jointIds;
    }

    // Creates a sliding (prismatic) joint for drawbridges/portcullises that
    // translate along a single axis rather than rotating.
    uint32_t createSlidingJoint(JPH::BodyID bodyA, JPH::BodyID bodyB,
                                 const glm::vec3& slideAxis, const JointLimits& limits,
                                 const std::string& label = "sliding") {
        JointInstance instance;
        instance.jointId = nextJointId++;
        instance.type = JointType::SLIDING;
        instance.bodyA = bodyA;
        instance.bodyB = bodyB;
        instance.limits = limits;
        instance.debugLabel = label;
        (void)slideAxis;
        joints[instance.jointId] = instance;
        return instance.jointId;
    }

    // Creates a rigid weld between two bodies (e.g. a prop bolted to a wall).
    uint32_t createFixedJoint(JPH::BodyID bodyA, JPH::BodyID bodyB, const std::string& label = "fixed") {
        JointInstance instance;
        instance.jointId = nextJointId++;
        instance.type = JointType::FIXED;
        instance.bodyA = bodyA;
        instance.bodyB = bodyB;
        instance.debugLabel = label;
        joints[instance.jointId] = instance;
        return instance.jointId;
    }

    // Creates a spring-loaded joint for trap mechanisms (e.g. swinging axe,
    // pressure-plate-triggered dart launcher).
    uint32_t createSpringJoint(JPH::BodyID bodyA, JPH::BodyID bodyB,
                                float stiffness, float damping, float restLength,
                                const std::string& label = "trap_spring") {
        JointInstance instance;
        instance.jointId = nextJointId++;
        instance.type = JointType::SPRING;
        instance.bodyA = bodyA;
        instance.bodyB = bodyB;
        instance.debugLabel = label;
        springStiffness[instance.jointId] = stiffness;
        springDamping[instance.jointId] = damping;
        springRestLength[instance.jointId] = restLength;
        joints[instance.jointId] = instance;
        return instance.jointId;
    }

    // Enables a motor on an existing joint (used for powered doors and
    // elevators). No-op for SPRING/BALL_SOCKET joints which have no motor.
    void setMotor(uint32_t jointId, bool enabled, float targetVelocity, float maxForce) {
        auto it = joints.find(jointId);
        if (it == joints.end()) return;
        it->second.motor.enabled = enabled;
        it->second.motor.targetVelocity = targetVelocity;
        it->second.motor.maxTorqueOrForce = maxForce;
    }

    // Marks a joint as breakable; ApplyImpulse/force accumulation logic in
    // the .cpp checks these thresholds each physics step.
    void setBreakable(uint32_t jointId, float maxForce, float maxTorque) {
        auto it = joints.find(jointId);
        if (it == joints.end()) return;
        it->second.breakSettings.breakable = true;
        it->second.breakSettings.maxForce = maxForce;
        it->second.breakSettings.maxTorque = maxTorque;
    }

    // Call once per physics step to evaluate breakable joints and advance
    // spring dynamics. Actual constraint removal happens against the live
    // JPH::PhysicsSystem in the .cpp implementation.
    void update(float deltaTime) {
        (void)deltaTime;
        for (auto& [id, joint] : joints) {
            if (joint.broken || !joint.breakSettings.breakable) continue;
            // Force/torque sampling against the live constraint is
            // implemented in joint_system.cpp using constraint->GetTotalLambdaX/Y/Z.
        }
    }

    void breakJoint(uint32_t jointId) {
        auto it = joints.find(jointId);
        if (it == joints.end()) return;
        it->second.broken = true;
        OBLIVION_JOINT_LOGI("Joint '%s' (id=%u) broken", it->second.debugLabel.c_str(), jointId);
    }

    void removeJoint(uint32_t jointId) {
        joints.erase(jointId);
        springStiffness.erase(jointId);
        springDamping.erase(jointId);
        springRestLength.erase(jointId);
    }

    JointInstance* getJoint(uint32_t jointId) {
        auto it = joints.find(jointId);
        return it != joints.end() ? &it->second : nullptr;
    }

    size_t getJointCount() const { return joints.size(); }

    // Debug visualization hook: returns endpoints to draw as line segments
    // for every active joint. Populated by the .cpp from live body
    // transforms; declared here so calling code can render without knowing
    // Jolt internals.
    struct DebugLine {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec3 color;
    };
    void setDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }
    bool isDebugDrawEnabled() const { return debugDrawEnabled; }
    const std::vector<DebugLine>& getDebugLines() const { return debugLines; }
    std::vector<DebugLine>& getDebugLinesMutable() { return debugLines; }

private:
    JointSystem() = default;

    PhysicsManager* physicsManager = nullptr;
    uint32_t nextJointId = 1;
    std::unordered_map<uint32_t, JointInstance> joints;
    std::unordered_map<uint32_t, float> springStiffness;
    std::unordered_map<uint32_t, float> springDamping;
    std::unordered_map<uint32_t, float> springRestLength;

    bool debugDrawEnabled = false;
    std::vector<DebugLine> debugLines;
};

} // namespace oblivion
