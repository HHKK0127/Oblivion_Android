#pragma once

#include "collision_world.h"
#include <glm/glm.hpp>

// ============================================
// Phase 30 Step 12: CharacterController
// Capsule-based character with substep movement
// ============================================

struct GroundInfo {
    bool isGrounded = false;
    float groundHeight = 0.0f;
    glm::vec3 groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    float groundSlope = 1.0f;  // cos(angle), 1.0 = flat
};

class CharacterController {
public:
    CharacterController();
    ~CharacterController();

    // Initialize with collision world
    void init(CollisionWorld* world, const glm::vec3& position, float radius, float height);

    // Movement
    void move(const glm::vec3& delta);

    // Update (call each frame)
    void update(float deltaTime);

    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getVelocity() const { return velocity; }
    bool isGrounded() const { return groundInfo.isGrounded; }
    const GroundInfo& getGroundInfo() const { return groundInfo; }

    // Setters
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    void setGravity(const glm::vec3& g) { gravity = g; }
    void setStepHeight(float h) { stepHeight = h; }
    void setMaxSlopeAngle(float degrees);

private:
    static constexpr int SUBSTEPS = 4;
    static constexpr int GROUND_RAYS = 5;  // Center + 4 corners
    static constexpr float GROUND_CHECK_DIST = 0.3f;
    static constexpr float SKIN_WIDTH = 0.02f;

    CollisionWorld* collisionWorld = nullptr;
    int32_t bodyId = -1;

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);

    float capsuleRadius = 0.4f;
    float capsuleHeight = 1.8f;
    float stepHeight = 0.3f;
    float maxSlopeCos = 0.7f;  // ~45 degrees

    GroundInfo groundInfo;

    // Internal movement with substeps
    void moveWithSubsteps(const glm::vec3& delta);

    // Resolve collision with a single body
    bool resolveCollision(const glm::vec3& position, float radius, float height,
                          int32_t otherId, glm::vec3& correction);

    // Ground detection
    GroundInfo checkGround(const glm::vec3& pos);

    // Swept sphere test
    bool sweptSphereTest(const glm::vec3& start, const glm::vec3& delta, float radius,
                          int32_t ignoreId, float& t, glm::vec3& normal);

    // Wall sliding
    glm::vec3 slideAlongWall(const glm::vec3& delta, const glm::vec3& wallNormal);
};
