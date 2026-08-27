#include "collision_world.h"
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================
// Phase 30 Step 11: CollisionWorld
// ============================================

// Static dispatch table initialization
NarrowPhaseFunc CollisionWorld::dispatchTable[static_cast<uint32_t>(ShapeType::ST_COUNT)]
                                              [static_cast<uint32_t>(ShapeType::ST_COUNT)];

namespace {
    struct DispatchTableInitializer {
        DispatchTableInitializer() {
            CollisionWorld::initDispatchTable();
        }
    };
    static DispatchTableInitializer g_dispatchInit;
}

void CollisionWorld::initDispatchTable() {
    // Initialize all entries to nullptr
    for (uint32_t i = 0; i < static_cast<uint32_t>(ShapeType::ST_COUNT); i++) {
        for (uint32_t j = 0; j < static_cast<uint32_t>(ShapeType::ST_COUNT); j++) {
            dispatchTable[i][j] = nullptr;
        }
    }

    // SPHERE row
    dispatchTable[static_cast<uint32_t>(ShapeType::SPHERE)]
                 [static_cast<uint32_t>(ShapeType::SPHERE)] = npSphereSphere;
    dispatchTable[static_cast<uint32_t>(ShapeType::SPHERE)]
                 [static_cast<uint32_t>(ShapeType::BOX)] = npSphereBox;
    dispatchTable[static_cast<uint32_t>(ShapeType::SPHERE)]
                 [static_cast<uint32_t>(ShapeType::CAPSULE)] = npSphereCapsule;
    dispatchTable[static_cast<uint32_t>(ShapeType::SPHERE)]
                 [static_cast<uint32_t>(ShapeType::CONVEX)] = npSphereConvex;

    // BOX row
    dispatchTable[static_cast<uint32_t>(ShapeType::BOX)]
                 [static_cast<uint32_t>(ShapeType::SPHERE)] = npSphereBox;
    dispatchTable[static_cast<uint32_t>(ShapeType::BOX)]
                 [static_cast<uint32_t>(ShapeType::BOX)] = npBoxBox;
    dispatchTable[static_cast<uint32_t>(ShapeType::BOX)]
                 [static_cast<uint32_t>(ShapeType::CAPSULE)] = npBoxCapsule;
    dispatchTable[static_cast<uint32_t>(ShapeType::BOX)]
                 [static_cast<uint32_t>(ShapeType::CONVEX)] = npBoxConvex;

    // CAPSULE row
    dispatchTable[static_cast<uint32_t>(ShapeType::CAPSULE)]
                 [static_cast<uint32_t>(ShapeType::SPHERE)] = npSphereCapsule;
    dispatchTable[static_cast<uint32_t>(ShapeType::CAPSULE)]
                 [static_cast<uint32_t>(ShapeType::BOX)] = npBoxCapsule;
    dispatchTable[static_cast<uint32_t>(ShapeType::CAPSULE)]
                 [static_cast<uint32_t>(ShapeType::CAPSULE)] = npCapsuleCapsule;
    dispatchTable[static_cast<uint32_t>(ShapeType::CAPSULE)]
                 [static_cast<uint32_t>(ShapeType::CONVEX)] = npCapsuleConvex;

    // CONVEX row
    dispatchTable[static_cast<uint32_t>(ShapeType::CONVEX)]
                 [static_cast<uint32_t>(ShapeType::SPHERE)] = npSphereConvex;
    dispatchTable[static_cast<uint32_t>(ShapeType::CONVEX)]
                 [static_cast<uint32_t>(ShapeType::BOX)] = npBoxConvex;
    dispatchTable[static_cast<uint32_t>(ShapeType::CONVEX)]
                 [static_cast<uint32_t>(ShapeType::CAPSULE)] = npCapsuleConvex;
    dispatchTable[static_cast<uint32_t>(ShapeType::CONVEX)]
                 [static_cast<uint32_t>(ShapeType::CONVEX)] = npConvexConvex;

    // MESH entries remain nullptr (handled separately)
}

CollisionWorld::CollisionWorld() {
    bodies.reserve(256);
    freeSlots.reserve(64);
}

CollisionWorld::~CollisionWorld() {
}

int32_t CollisionWorld::addBody(const CollisionBody& body) {
    int32_t id;

    if (!freeSlots.empty()) {
        id = freeSlots.back();
        freeSlots.pop_back();
        bodies[id] = body;
        bodies[id].id = id;
    } else {
        id = static_cast<int32_t>(bodies.size());
        bodies.push_back(body);
        bodies[id].id = id;
    }

    // Insert into broad phase
    AABB aabb = computeBodyAABB(bodies[id]);
    bodies[id].treeNodeIndex = broadPhase.insert(aabb, id);

    return id;
}

void CollisionWorld::removeBody(int32_t bodyId) {
    if (bodyId < 0 || bodyId >= static_cast<int32_t>(bodies.size())) return;

    if (bodies[bodyId].treeNodeIndex >= 0) {
        broadPhase.remove(bodies[bodyId].treeNodeIndex);
    }

    bodies[bodyId].id = -1;
    bodies[bodyId].isActive = false;
    freeSlots.push_back(bodyId);
}

CollisionBody* CollisionWorld::getBody(int32_t bodyId) {
    if (bodyId < 0 || bodyId >= static_cast<int32_t>(bodies.size())) return nullptr;
    if (bodies[bodyId].id == -1) return nullptr;
    return &bodies[bodyId];
}

const CollisionBody* CollisionWorld::getBody(int32_t bodyId) const {
    if (bodyId < 0 || bodyId >= static_cast<int32_t>(bodies.size())) return nullptr;
    if (bodies[bodyId].id == -1) return nullptr;
    return &bodies[bodyId];
}

void CollisionWorld::updateBody(int32_t bodyId, const glm::vec3& position) {
    CollisionBody* body = getBody(bodyId);
    if (!body) return;

    body->position = position;

    // Update AABB in broad phase
    if (body->treeNodeIndex >= 0) {
        AABB aabb = computeBodyAABB(*body);
        broadPhase.update(body->treeNodeIndex, aabb);
    }
}

void CollisionWorld::step(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    // Integrate velocities
    integrateVelocities(deltaTime);

    // Broad phase: collect potentially colliding pairs
    std::vector<std::pair<int32_t, int32_t>> pairs;
    broadPhaseCollect(pairs);

    // Narrow phase: test specific shape pairs
    contacts.clear();
    for (const auto& pair : pairs) {
        const CollisionBody& a = bodies[pair.first];
        const CollisionBody& b = bodies[pair.second];

        if (!a.isActive || !b.isActive) continue;

        // Check collision filter
        if ((a.collisionGroup & b.collisionFilter) == 0 &&
            (b.collisionGroup & a.collisionFilter) == 0) {
            continue;
        }

        narrowPhaseTest(a, b);
    }

    // Resolve contacts
    resolveContacts(deltaTime);
}

void CollisionWorld::integrateVelocities(float deltaTime) {
    for (auto& body : bodies) {
        if (body.id == -1 || body.isStatic || !body.isActive) continue;

        // Apply gravity
        body.linearVelocity += gravity * deltaTime;

        // Update position
        body.position += body.linearVelocity * deltaTime;

        // Update AABB
        if (body.treeNodeIndex >= 0) {
            AABB aabb = computeBodyAABB(body);
            broadPhase.update(body.treeNodeIndex, aabb);
        }
    }
}

void CollisionWorld::resolveContacts(float deltaTime) {
    for (size_t i = 0; i < contacts.size(); i++) {
        const ContactPoint& contact = contacts.data()[i];

        CollisionBody* bodyA = getBody(contact.bodyA);
        CollisionBody* bodyB = getBody(contact.bodyB);

        if (!bodyA || !bodyB) continue;

        // Skip if both are static
        if (bodyA->isStatic && bodyB->isStatic) continue;

        // Skip triggers
        if (bodyA->isTrigger || bodyB->isTrigger) continue;

        // Position correction (separate overlapping bodies)
        float totalInvMass = 0.0f;
        if (!bodyA->isStatic) totalInvMass += 1.0f / bodyA->mass;
        if (!bodyB->isStatic) totalInvMass += 1.0f / bodyB->mass;

        if (totalInvMass > 0.0f) {
            float correction = contact.penetration / totalInvMass * 0.8f;  // 80% correction

            if (!bodyA->isStatic) {
                bodyA->position -= contact.normal * (correction / bodyA->mass);
            }
            if (!bodyB->isStatic) {
                bodyB->position += contact.normal * (correction / bodyB->mass);
            }
        }

        // Velocity correction (bounce)
        glm::vec3 relativeVelocity = bodyB->linearVelocity - bodyA->linearVelocity;
        float velocityAlongNormal = glm::dot(relativeVelocity, contact.normal);

        // Only resolve if bodies are moving toward each other
        if (velocityAlongNormal > 0.0f) continue;

        float restitution = std::min(bodyA->restitution, bodyB->restitution);
        float j = -(1.0f + restitution) * velocityAlongNormal;
        j /= totalInvMass;

        glm::vec3 impulse = contact.normal * j;

        if (!bodyA->isStatic) {
            bodyA->linearVelocity -= impulse / bodyA->mass;
        }
        if (!bodyB->isStatic) {
            bodyB->linearVelocity += impulse / bodyB->mass;
        }

        // Friction
        glm::vec3 tangent = relativeVelocity - contact.normal * velocityAlongNormal;
        float tangentLen = glm::length(tangent);
        if (tangentLen > 1e-6f) {
            tangent = tangent / tangentLen;
            float frictionCoeff = (bodyA->friction + bodyB->friction) * 0.5f;
            float jt = -glm::dot(relativeVelocity, tangent);
            jt /= totalInvMass;

            glm::vec3 frictionImpulse;
            if (fabsf(jt) < j * frictionCoeff) {
                frictionImpulse = tangent * jt;
            } else {
                frictionImpulse = tangent * (-j * frictionCoeff);
            }

            if (!bodyA->isStatic) {
                bodyA->linearVelocity -= frictionImpulse / bodyA->mass;
            }
            if (!bodyB->isStatic) {
                bodyB->linearVelocity += frictionImpulse / bodyB->mass;
            }
        }

        // Update AABBs
        if (!bodyA->isStatic && bodyA->treeNodeIndex >= 0) {
            AABB aabb = computeBodyAABB(*bodyA);
            broadPhase.update(bodyA->treeNodeIndex, aabb);
        }
        if (!bodyB->isStatic && bodyB->treeNodeIndex >= 0) {
            AABB aabb = computeBodyAABB(*bodyB);
            broadPhase.update(bodyB->treeNodeIndex, aabb);
        }
    }
}

void CollisionWorld::broadPhaseCollect(std::vector<std::pair<int32_t, int32_t>>& pairs) {
    pairs.clear();

    for (const auto& body : bodies) {
        if (body.id == -1 || !body.isActive) continue;

        // Query broad phase for overlapping bodies
        std::vector<int32_t> candidates;
        AABB aabb = computeBodyAABB(body);
        broadPhase.query(aabb, candidates);

        for (int32_t candidateId : candidates) {
            if (candidateId <= body.id) continue;  // Avoid duplicate pairs

            const CollisionBody& other = bodies[candidateId];
            if (other.id == -1 || !other.isActive) continue;

            pairs.push_back({body.id, candidateId});
        }
    }
}

void CollisionWorld::narrowPhaseTest(const CollisionBody& a, const CollisionBody& b) {
    uint32_t typeA = static_cast<uint32_t>(a.shapeType);
    uint32_t typeB = static_cast<uint32_t>(b.shapeType);

    if (typeA >= static_cast<uint32_t>(ShapeType::ST_COUNT) ||
        typeB >= static_cast<uint32_t>(ShapeType::ST_COUNT)) {
        return;
    }

    NarrowPhaseFunc func = dispatchTable[typeA][typeB];
    if (func) {
        func(a, b, contacts);
    }
}

AABB CollisionWorld::computeBodyAABB(const CollisionBody& body) const {
    glm::vec3 halfSize;

    switch (body.shapeType) {
        case ShapeType::SPHERE:
            halfSize = glm::vec3(body.radius, body.radius, body.radius);
            break;
        case ShapeType::BOX:
            halfSize = body.halfExtents;
            break;
        case ShapeType::CAPSULE:
            halfSize = glm::vec3(body.radius, body.height * 0.5f + body.radius, body.radius);
            break;
        case ShapeType::CONVEX:
        case ShapeType::MESH:
            // Use half extents if available, otherwise use radius
            if (body.halfExtents.x > 0.0f) {
                halfSize = body.halfExtents;
            } else {
                halfSize = glm::vec3(body.radius, body.radius, body.radius);
            }
            break;
        default:
            halfSize = glm::vec3(body.radius, body.radius, body.radius);
            break;
    }

    return AABB(body.position - halfSize, body.position + halfSize);
}

void CollisionWorld::queryAABB(const AABB& bounds, std::vector<int32_t>& results) const {
    broadPhase.query(bounds, results);
}

void CollisionWorld::querySphere(const glm::vec3& center, float radius,
                                  std::vector<int32_t>& results) const {
    AABB aabb(center - glm::vec3(radius, radius, radius),
              center + glm::vec3(radius, radius, radius));
    broadPhase.query(aabb, results);

    // Filter by actual sphere intersection
    results.erase(
        std::remove_if(results.begin(), results.end(),
            [&](int32_t id) {
                const CollisionBody* body = getBody(id);
                if (!body) return true;
                float dist = glm::length(body->position - center);
                return dist > radius + body->radius;
            }),
        results.end()
    );
}

void CollisionWorld::raycast(const glm::vec3& origin, const glm::vec3& direction,
                              float maxDistance, std::vector<int32_t>& results) const {
    broadPhase.raycast(origin, direction, maxDistance, results);
}

// ============================================
// Narrow Phase Implementations
// ============================================

bool npSphereSphere(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    glm::vec3 delta = b.position - a.position;
    float distSq = glm::dot(delta, delta);
    float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum) return false;

    float dist = sqrtf(distSq);
    ContactPoint contact;
    contact.bodyA = a.id;
    contact.bodyB = b.id;

    if (dist > 1e-6f) {
        contact.normal = delta / dist;
    } else {
        contact.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        dist = 0.0f;
    }

    contact.penetration = radiusSum - dist;
    contact.pointA = a.position + contact.normal * a.radius;
    contact.pointB = b.position - contact.normal * b.radius;

    return contacts.add(contact);
}

bool npSphereBox(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // a = sphere, b = box
    glm::vec3 localPos = a.position - b.position;

    // Clamp to box extents
    glm::vec3 closest;
    closest.x = std::max(-b.halfExtents.x, std::min(localPos.x, b.halfExtents.x));
    closest.y = std::max(-b.halfExtents.y, std::min(localPos.y, b.halfExtents.y));
    closest.z = std::max(-b.halfExtents.z, std::min(localPos.z, b.halfExtents.z));

    glm::vec3 delta = localPos - closest;
    float distSq = glm::dot(delta, delta);

    if (distSq > a.radius * a.radius) return false;

    float dist = sqrtf(distSq);

    ContactPoint contact;
    contact.bodyA = a.id;
    contact.bodyB = b.id;

    if (dist > 1e-6f) {
        contact.normal = delta / dist;
    } else {
        contact.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        dist = 0.0f;
    }

    contact.penetration = a.radius - dist;
    contact.pointA = a.position - contact.normal * a.radius;
    contact.pointB = b.position + closest;

    return contacts.add(contact);
}

bool npSphereCapsule(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // a = sphere, b = capsule
    // Capsule: line segment from (0, -height/2, 0) to (0, height/2, 0) with radius
    glm::vec3 capsuleA = b.position + glm::vec3(0.0f, -b.height * 0.5f, 0.0f);
    glm::vec3 capsuleB = b.position + glm::vec3(0.0f, b.height * 0.5f, 0.0f);

    // Find closest point on capsule segment to sphere center
    glm::vec3 seg = capsuleB - capsuleA;
    float t = glm::dot(a.position - capsuleA, seg) / glm::dot(seg, seg);
    t = std::max(0.0f, std::min(1.0f, t));

    glm::vec3 closest = capsuleA + seg * t;
    glm::vec3 delta = a.position - closest;
    float distSq = glm::dot(delta, delta);
    float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum) return false;

    float dist = sqrtf(distSq);

    ContactPoint contact;
    contact.bodyA = a.id;
    contact.bodyB = b.id;

    if (dist > 1e-6f) {
        contact.normal = delta / dist;
    } else {
        contact.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        dist = 0.0f;
    }

    contact.penetration = radiusSum - dist;
    contact.pointA = a.position - contact.normal * a.radius;
    contact.pointB = closest + contact.normal * b.radius;

    return contacts.add(contact);
}

bool npSphereConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // Simplified: treat convex as sphere for now
    return npSphereSphere(a, b, contacts);
}

bool npBoxBox(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // AABB vs AABB (no rotation support yet)
    glm::vec3 delta = b.position - a.position;
    glm::vec3 overlap;
    overlap.x = (a.halfExtents.x + b.halfExtents.x) - fabsf(delta.x);
    overlap.y = (a.halfExtents.y + b.halfExtents.y) - fabsf(delta.y);
    overlap.z = (a.halfExtents.z + b.halfExtents.z) - fabsf(delta.z);

    if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) return false;

    ContactPoint contact;
    contact.bodyA = a.id;
    contact.bodyB = b.id;

    // Find minimum overlap axis
    if (overlap.x <= overlap.y && overlap.x <= overlap.z) {
        contact.normal = glm::vec3(delta.x > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
        contact.penetration = overlap.x;
    } else if (overlap.y <= overlap.x && overlap.y <= overlap.z) {
        contact.normal = glm::vec3(0.0f, delta.y > 0.0f ? 1.0f : -1.0f, 0.0f);
        contact.penetration = overlap.y;
    } else {
        contact.normal = glm::vec3(0.0f, 0.0f, delta.z > 0.0f ? 1.0f : -1.0f);
        contact.penetration = overlap.z;
    }

    contact.pointA = glm::vec3(
        a.position.x + contact.normal.x * a.halfExtents.x,
        a.position.y + contact.normal.y * a.halfExtents.y,
        a.position.z + contact.normal.z * a.halfExtents.z
    );
    contact.pointB = glm::vec3(
        b.position.x - contact.normal.x * b.halfExtents.x,
        b.position.y - contact.normal.y * b.halfExtents.y,
        b.position.z - contact.normal.z * b.halfExtents.z
    );

    return contacts.add(contact);
}

bool npBoxCapsule(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // a = box, b = capsule
    // Treat capsule as sphere for simplification
    CollisionBody sphereB = b;
    sphereB.shapeType = ShapeType::SPHERE;
    return npSphereBox(sphereB, a, contacts);
}

bool npBoxConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // Simplified: treat convex as box
    return npBoxBox(a, b, contacts);
}

bool npCapsuleCapsule(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // Two capsules: find closest points between two line segments
    glm::vec3 aA = a.position + glm::vec3(0.0f, -a.height * 0.5f, 0.0f);
    glm::vec3 aB = a.position + glm::vec3(0.0f, a.height * 0.5f, 0.0f);
    glm::vec3 bA = b.position + glm::vec3(0.0f, -b.height * 0.5f, 0.0f);
    glm::vec3 bB = b.position + glm::vec3(0.0f, b.height * 0.5f, 0.0f);

    // Find closest points on two segments
    glm::vec3 d1 = aB - aA;
    glm::vec3 d2 = bB - bA;
    glm::vec3 r = aA - bA;

    float a_dot = glm::dot(d1, d1);
    float b_dot = glm::dot(d2, d2);
    float c_dot = glm::dot(d1, d2);
    float d_dot = glm::dot(d1, r);
    float e_dot = glm::dot(d2, r);

    float denom = a_dot * b_dot - c_dot * c_dot;
    float s, t;

    if (denom > 1e-6f) {
        s = std::max(0.0f, std::min(1.0f, (c_dot * e_dot - b_dot * d_dot) / denom));
    } else {
        s = 0.0f;
    }

    // Guard against b_dot being zero (d2 is zero vector)
    if (b_dot > 1e-6f) {
        t = (c_dot * s + e_dot) / b_dot;
        t = std::max(0.0f, std::min(1.0f, t));
    } else {
        t = 0.0f;
    }

    // Recompute s with clamped t
    if (a_dot > 1e-6f) {
        s = std::max(0.0f, std::min(1.0f, (c_dot * t - d_dot) / a_dot));
    } else {
        s = 0.0f;
    }

    glm::vec3 closestA = aA + d1 * s;
    glm::vec3 closestB = bA + d2 * t;

    glm::vec3 delta = closestB - closestA;
    float distSq = glm::dot(delta, delta);
    float radiusSum = a.radius + b.radius;

    if (distSq > radiusSum * radiusSum) return false;

    float dist = sqrtf(distSq);

    ContactPoint contact;
    contact.bodyA = a.id;
    contact.bodyB = b.id;

    if (dist > 1e-6f) {
        contact.normal = delta / dist;
    } else {
        contact.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        dist = 0.0f;
    }

    contact.penetration = radiusSum - dist;
    contact.pointA = closestA + contact.normal * a.radius;
    contact.pointB = closestB - contact.normal * b.radius;

    return contacts.add(contact);
}

bool npCapsuleConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // Simplified: treat convex as sphere
    CollisionBody sphereB = b;
    sphereB.shapeType = ShapeType::SPHERE;
    return npSphereCapsule(sphereB, a, contacts);
}

bool npConvexConvex(const CollisionBody& a, const CollisionBody& b, ContactBuffer& contacts) {
    // Simplified: treat as sphere-sphere
    return npSphereSphere(a, b, contacts);
}
