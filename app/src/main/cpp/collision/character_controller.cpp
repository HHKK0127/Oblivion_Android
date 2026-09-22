#include "character_controller.h"
#include <cmath>
#include <algorithm>

// ============================================
// Phase 30 Step 12: CharacterController
// ============================================

namespace {

// Ray vs a single static body. Boxes use a slab test on their half extents;
// every other shape is treated as a sphere of `radius`.
bool raycast_static_body(const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                         const CollisionBody& body, float& t, glm::vec3& normal) {
    if (body.shapeType == ShapeType::BOX) {
        glm::vec3 minB = body.position - body.halfExtents;
        glm::vec3 maxB = body.position + body.halfExtents;

        // The bundled GLM stub has no vec3 operator[], so work on raw floats.
        const float o[3] = { origin.x, origin.y, origin.z };
        const float d[3] = { dir.x, dir.y, dir.z };
        const float lo[3] = { minB.x, minB.y, minB.z };
        const float hi[3] = { maxB.x, maxB.y, maxB.z };

        float tmin = 0.0f;
        float tmax = maxDist;
        int axis = -1;
        float sign = 0.0f;

        for (int i = 0; i < 3; i++) {
            if (std::fabs(d[i]) < 1e-6f) {
                if (o[i] < lo[i] || o[i] > hi[i]) return false;
                continue;
            }

            float inv = 1.0f / d[i];
            float t1 = (lo[i] - o[i]) * inv;
            float t2 = (hi[i] - o[i]) * inv;
            float tNear = std::min(t1, t2);
            float tFar = std::max(t1, t2);

            if (tNear > tmin) {
                tmin = tNear;
                axis = i;
                sign = (t1 < t2) ? -1.0f : 1.0f;
            }
            if (tFar < tmax) tmax = tFar;
            if (tmin > tmax) return false;
        }

        // axis < 0 means the origin already started inside the box.
        if (axis < 0) return false;

        t = tmin;
        if (axis == 0)      normal = glm::vec3(sign, 0.0f, 0.0f);
        else if (axis == 1) normal = glm::vec3(0.0f, sign, 0.0f);
        else                normal = glm::vec3(0.0f, 0.0f, sign);
        return true;
    }

    float r = body.radius;
    glm::vec3 oc = origin - body.position;
    float b = glm::dot(oc, dir);
    float c = glm::dot(oc, oc) - r * r;
    float discriminant = b * b - c;
    if (discriminant < 0.0f) return false;

    float sq = std::sqrt(discriminant);
    float hitT = -b - sq;
    if (hitT < 0.0f) hitT = -b + sq;  // origin started inside the sphere
    if (hitT < 0.0f || hitT > maxDist) return false;

    t = hitT;
    glm::vec3 n = origin + dir * t - body.position;
    float len = glm::length(n);
    normal = (len > 1e-6f) ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
    return true;
}

}  // namespace

CharacterController::CharacterController() {
}

CharacterController::~CharacterController() {
    if (bodyId >= 0 && collisionWorld) {
        collisionWorld->removeBody(bodyId);
    }
}

void CharacterController::init(CollisionWorld* world, const glm::vec3& pos, float radius, float height) {
    collisionWorld = world;
    position = pos;
    capsuleRadius = radius;
    capsuleHeight = height;

    // Create capsule body
    if (collisionWorld) {
        CollisionBody body;
        body.shapeType = ShapeType::CAPSULE;
        body.position = pos;
        body.radius = radius;
        body.height = height;
        body.isStatic = false;
        body.isTrigger = false;
        body.mass = 1.0f;
        body.friction = 0.5f;
        body.restitution = 0.0f;

        bodyId = collisionWorld->addBody(body);
    }
}

void CharacterController::move(const glm::vec3& delta) {
    moveWithSubsteps(delta);
}

void CharacterController::moveWithSubsteps(const glm::vec3& delta) {
    glm::vec3 remaining = delta;
    float totalLen = glm::length(delta);

    if (totalLen < 1e-6f) return;

    glm::vec3 stepDelta = delta / static_cast<float>(SUBSTEPS);

    for (int i = 0; i < SUBSTEPS; i++) {
        // Try to move with collision resolution
        glm::vec3 newPos = position + stepDelta;

        if (collisionWorld) {
            // Check for collisions at the new position
            std::vector<int32_t> candidates;
            AABB testAABB(newPos - glm::vec3(capsuleRadius + SKIN_WIDTH,
                                             capsuleHeight * 0.5f + capsuleRadius + SKIN_WIDTH,
                                             capsuleRadius + SKIN_WIDTH),
                          newPos + glm::vec3(capsuleRadius + SKIN_WIDTH,
                                             capsuleHeight * 0.5f + capsuleRadius + SKIN_WIDTH,
                                             capsuleRadius + SKIN_WIDTH));
            collisionWorld->queryAABB(testAABB, candidates);

            bool resolved = false;
            for (int32_t otherId : candidates) {
                if (otherId == bodyId) continue;

                const CollisionBody* other = collisionWorld->getBody(otherId);
                if (!other || other->isStatic == false) continue;

                glm::vec3 correction;
                if (resolveCollision(newPos, capsuleRadius, capsuleHeight, otherId, correction)) {
                    newPos += correction;
                    resolved = true;
                }
            }

            // Update body position
            collisionWorld->updateBody(bodyId, newPos);
        }

        position = newPos;
    }
}

bool CharacterController::resolveCollision(const glm::vec3& pos, float radius, float height,
                                            int32_t otherId, glm::vec3& correction) {
    const CollisionBody* other = collisionWorld->getBody(otherId);
    if (!other) return false;

    correction = glm::vec3(0.0f, 0.0f, 0.0f);

    // The capsule is approximated by an axis-aligned box: half the capsule height
    // plus the cap radius vertically, and the capsule radius horizontally.
    float halfY = height * 0.5f + radius;

    if (other->shapeType == ShapeType::BOX) {
        // A box has no radius, so the separation has to come from its half extents.
        glm::vec3 otherHalf = other->halfExtents;
        glm::vec3 delta = pos - other->position;

        glm::vec3 overlap(radius + otherHalf.x - std::fabs(delta.x),
                          halfY + otherHalf.y - std::fabs(delta.y),
                          radius + otherHalf.z - std::fabs(delta.z));

        if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) return false;

        // Push out along the axis of least penetration.
        const float ov[3] = { overlap.x, overlap.y, overlap.z };
        const float dl[3] = { delta.x, delta.y, delta.z };

        int axis = 0;
        if (ov[1] < ov[axis]) axis = 1;
        if (ov[2] < ov[axis]) axis = 2;

        float push = (dl[axis] >= 0.0f) ? ov[axis] : -ov[axis];
        if (axis == 0)      correction = glm::vec3(push, 0.0f, 0.0f);
        else if (axis == 1) correction = glm::vec3(0.0f, push, 0.0f);
        else                correction = glm::vec3(0.0f, 0.0f, push);
        return true;
    }

    // Sphere-like shapes (SPHERE / CAPSULE / CONVEX): use the bounding sphere.
    float totalRadius = radius + other->radius + SKIN_WIDTH;
    glm::vec3 delta = other->position - pos;
    float distSq = glm::dot(delta, delta);

    if (distSq >= totalRadius * totalRadius) return false;

    float dist = sqrtf(distSq);
    if (dist < 1e-6f) {
        // Overlapping at same position, push upward
        correction = glm::vec3(0.0f, totalRadius, 0.0f);
        return true;
    }

    // Push out along the separation vector
    correction = delta * ((totalRadius - dist) / dist);
    return true;
}

GroundInfo CharacterController::checkGround(const glm::vec3& pos) {
    GroundInfo info;

    if (!collisionWorld) return info;

    // Cast rays downward from just above the character's feet.
    float halfY = capsuleHeight * 0.5f + capsuleRadius;
    float footY = pos.y - halfY + SKIN_WIDTH;
    float rayLength = GROUND_CHECK_DIST + SKIN_WIDTH;

    // Ray positions: center + 4 corners
    glm::vec3 rayOffsets[GROUND_RAYS] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(capsuleRadius * 0.7f, 0.0f, 0.0f),
        glm::vec3(-capsuleRadius * 0.7f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, capsuleRadius * 0.7f),
        glm::vec3(0.0f, 0.0f, -capsuleRadius * 0.7f)
    };

    glm::vec3 downDir = glm::vec3(0.0f, -1.0f, 0.0f);

    int hitCount = 0;
    bool haveHeight = false;

    for (int i = 0; i < GROUND_RAYS; i++) {
        glm::vec3 rayStart = glm::vec3(pos.x + rayOffsets[i].x, footY, pos.z + rayOffsets[i].z);

        std::vector<int32_t> candidates;
        AABB rayAABB(rayStart, rayStart + downDir * rayLength);
        collisionWorld->queryAABB(rayAABB, candidates);

        float closestDist = rayLength;
        glm::vec3 closestNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        bool hit = false;

        for (int32_t otherId : candidates) {
            if (otherId == bodyId) continue;

            const CollisionBody* other = collisionWorld->getBody(otherId);
            if (!other || !other->isStatic) continue;

            float t;
            glm::vec3 normal;
            if (raycast_static_body(rayStart, downDir, rayLength, *other, t, normal) && t < closestDist) {
                closestDist = t;
                closestNormal = normal;
                hit = true;
            }
        }

        if (hit) {
            // Height of the surface below this probe.
            float surfaceY = rayStart.y - closestDist;
            info.groundHeight = haveHeight ? std::max(info.groundHeight, surfaceY) : surfaceY;
            haveHeight = true;

            info.groundNormal += closestNormal;
            hitCount++;
        }
    }

    if (hitCount > 0) {
        info.groundNormal = info.groundNormal * (1.0f / static_cast<float>(hitCount));
        float normalLen = glm::length(info.groundNormal);
        if (normalLen > 1e-6f) {
            info.groundNormal = info.groundNormal / normalLen;
        }

        info.groundSlope = info.groundNormal.y;
        info.isGrounded = info.groundSlope >= maxSlopeCos;
    } else {
        info.isGrounded = false;
    }

    return info;
}

bool CharacterController::sweptSphereTest(const glm::vec3& start, const glm::vec3& delta, float radius,
                                            int32_t ignoreId, float& t, glm::vec3& normal) {
    if (!collisionWorld) return false;

    glm::vec3 end = start + delta;
    AABB testAABB(
        glm::vec3(std::min(start.x, end.x) - radius, std::min(start.y, end.y) - radius, std::min(start.z, end.z) - radius),
        glm::vec3(std::max(start.x, end.x) + radius, std::max(start.y, end.y) + radius, std::max(start.z, end.z) + radius)
    );

    std::vector<int32_t> candidates;
    collisionWorld->queryAABB(testAABB, candidates);

    float deltaLen = glm::length(delta);
    if (deltaLen < 1e-6f) return false;

    glm::vec3 dir = delta / deltaLen;

    bool hit = false;
    float minT = deltaLen;

    for (int32_t otherId : candidates) {
        if (otherId == ignoreId) continue;

        const CollisionBody* other = collisionWorld->getBody(otherId);
        if (!other || !other->isStatic) continue;

        // Simplified sphere-sphere sweep
        glm::vec3 toOther = other->position - start;
        float totalRadius = radius + other->radius;

        // Project onto direction
        float projection = glm::dot(toOther, dir);

        glm::vec3 closest = start + dir * projection;
        glm::vec3 toClosest = other->position - closest;
        float perpDistSq = glm::dot(toClosest, toClosest);

        if (perpDistSq > totalRadius * totalRadius) continue;

        float backtrack = sqrtf(totalRadius * totalRadius - perpDistSq);
        float tEnter = projection - backtrack;
        float tExit = projection + backtrack;

        if (tExit < 0.0f || tEnter > minT) continue;

        float hitT = std::max(0.0f, tEnter);
        if (hitT >= minT) continue;

        minT = hitT;
        glm::vec3 hitPoint = start + dir * hitT;
        normal = glm::normalize(hitPoint - other->position);
        if (glm::length(normal) < 1e-6f) {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        hit = true;
    }

    if (hit) {
        t = minT;
        return true;
    }
    return false;
}

glm::vec3 CharacterController::slideAlongWall(const glm::vec3& delta, const glm::vec3& wallNormal) {
    glm::vec3 parallel = delta - wallNormal * glm::dot(delta, wallNormal);
    return parallel;
}

void CharacterController::update(float deltaTime) {
    // Apply gravity
    if (!groundInfo.isGrounded) {
        velocity += gravity * deltaTime;
    }

    // Apply velocity
    if (glm::length(velocity) > 1e-6f) {
        move(velocity * deltaTime);
    }

    // Update ground info
    groundInfo = checkGround(position);

    // Snap to ground if grounded
    if (groundInfo.isGrounded && velocity.y < 0.0f) {
        velocity.y = 0.0f;
    }
}

void CharacterController::setMaxSlopeAngle(float degrees) {
    maxSlopeCos = std::cos(degrees * 3.14159265f / 180.0f);
}
