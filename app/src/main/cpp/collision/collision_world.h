#pragma once

#include "aabb_tree.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <functional>

// ============================================
// Phase 30 Step 11: CollisionWorld
// Broad-phase + Narrow-phase collision detection
// ============================================

enum class ShapeType : uint32_t {
    SPHERE = 0,
    BOX,
    CAPSULE,
    CONVEX,
    MESH,
    ST_COUNT  // Must be last
};

struct CollisionBody {
    int32_t id = -1;
    ShapeType shapeType = ShapeType::SPHERE;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);  // Euler angles
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

    // Shape parameters
    float radius = 0.0f;
    glm::vec3 halfExtents = glm::vec3(0.0f, 0.0f, 0.0f);
    float height = 0.0f;

    // Physics properties
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    glm::vec3 linearVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f, 0.0f, 0.0f);

    // Collision filter
    uint32_t collisionGroup = 0;
    uint32_t collisionFilter = 0;

    // State
    bool isStatic = true;
    bool isTrigger = false;
    bool isActive = true;

    // AABB tree node index
    int32_t treeNodeIndex = -1;

    // User data (e.g., entity ID)
    int32_t userData = -1;
};

struct ContactPoint {
    glm::vec3 pointA;
    glm::vec3 pointB;
    glm::vec3 normal;  // From A to B
    float penetration = 0.0f;
    int32_t bodyA = -1;
    int32_t bodyB = -1;
};

class ContactBuffer {
public:
    static constexpr size_t MAX_CONTACTS = 256;

    ContactBuffer() : count(0) {}

    void clear() { count = 0; }

    bool add(const ContactPoint& c) {
        if (count >= MAX_CONTACTS) return false;
        contacts[count++] = c;
        return true;
    }

    ContactPoint* data() { return contacts; }
    const ContactPoint* data() const { return contacts; }
    size_t size() const { return count; }
    bool empty() const { return count == 0; }

private:
    ContactPoint contacts[MAX_CONTACTS];
    size_t count;
};

// Narrow phase function type
using NarrowPhaseFunc = bool(*)(const CollisionBody& a, const CollisionBody& b,
                                 ContactBuffer& contacts);

class CollisionWorld {
public:
    CollisionWorld();
    ~CollisionWorld();

    // Body management
    int32_t addBody(const CollisionBody& body);
    void removeBody(int32_t bodyId);
    CollisionBody* getBody(int32_t bodyId);
    const CollisionBody* getBody(int32_t bodyId) const;

    // Update body position and AABB
    void updateBody(int32_t bodyId, const glm::vec3& position);

    // Collision detection
    void step(float deltaTime);

    // Query
    void queryAABB(const AABB& bounds, std::vector<int32_t>& results) const;
    void querySphere(const glm::vec3& center, float radius, std::vector<int32_t>& results) const;
    void raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                 std::vector<int32_t>& results) const;

    // Contact access
    const ContactBuffer& getContacts() const { return contacts; }

    // Gravity
    void setGravity(const glm::vec3& gravity) { this->gravity = gravity; }
    glm::vec3 getGravity() const { return gravity; }

    // Statistics
    int32_t getBodyCount() const { return static_cast<int32_t>(bodies.size()); }
    int32_t getActiveContacts() const { return static_cast<int32_t>(contacts.size()); }

private:
    static constexpr int32_t NULL_BODY = -1;

    std::vector<CollisionBody> bodies;
    std::vector<int32_t> freeSlots;
    DynamicAABBTree broadPhase;
    ContactBuffer contacts;
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);

    // Narrow phase dispatch table
    static NarrowPhaseFunc dispatchTable[static_cast<uint32_t>(ShapeType::ST_COUNT)]
                                         [static_cast<uint32_t>(ShapeType::ST_COUNT)];

public:
    // Initialize dispatch table
    static void initDispatchTable();

    // Broad phase: collect potentially colliding pairs
    void broadPhaseCollect(std::vector<std::pair<int32_t, int32_t>>& pairs);

    // Narrow phase: test specific shape pairs
    void narrowPhaseTest(const CollisionBody& a, const CollisionBody& b);

    // AABB computation for body
    AABB computeBodyAABB(const CollisionBody& body) const;

    // Physics integration
    void integrateVelocities(float deltaTime);
    void resolveContacts(float deltaTime);
};

// Narrow phase implementations
bool npSphereSphere(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npSphereBox(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npSphereCapsule(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npSphereConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npBoxBox(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npBoxCapsule(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npBoxConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npCapsuleCapsule(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npCapsuleConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
bool npConvexConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts);
