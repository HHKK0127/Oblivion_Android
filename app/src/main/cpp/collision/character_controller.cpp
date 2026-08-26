#include "character_controller.h"
#include <cmath>
#include <algorithm>

// ============================================
// Phase 30 Step 12: CharacterController
// ============================================

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

    // Simple sphere-sphere collision for the capsule endpoints
    glm::vec3 topPos = pos + glm::vec3(0.0f, height * 0.5f, 0.0f);
    glm::vec3 bottomPos = pos - glm::vec3(0.0f, height * 0.5f, 0.0f);

    // Simplified: use bounding sphere check
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

    // Cast multiple rays downward from the character's base
    float baseY = pos.y - capsuleHeight * 0.5f;
    float rayLength = GROUND_CHECK_DIST + capsuleRadius;

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

    for (int i = 0; i < GROUND_RAYS; i++) {
        glm::vec3 rayStart = glm::vec3(pos.x + rayOffsets[i].x, baseY + capsuleRadius, pos.z + rayOffsets[i].z);

        if (!collisionWorld) break;

        std::vector<int32_t> candidates;
        AABB rayAABB(rayStart, rayStart + downDir * rayLength);
        collisionWorld->queryAABB(rayAABB, candidates);

        float closestDist = rayLength;
        glm::vec3 closestNormal = glm::vec3(0.0f, 1.0f, 0.0f);

        for (int32_t otherId : candidates) {
            if (otherId == bodyId) continue;

            const CollisionBody* other = collisionWorld->getBody(otherId);
            if (!other || !other->isStatic) continue;

            // Simplified: check sphere-ground collision
            glm::vec3 spherePos = glm::vec3(rayStart.x, baseY, rayStart.z);
            float sphereRadius = capsuleRadius;
            float totalRadius = sphereRadius + other->radius;

            // Ray-sphere intersection
            glm::vec3 oc = spherePos - rayStart;
            float b = glm::dot(oc, downDir);
            float c = glm::dot(oc, oc) - totalRadius * totalRadius;
            float discriminant = b * b - c;

            if (discriminant < 0.0f) continue;

            float t = -b - sqrtf(discriminant);
            if (t < 0.0f || t >= closestDist) continue;

            closestDist = t;
            closestNormal = glm::vec3(0.0f, 1.0f, 0.0f);  // Simplified ground normal
        }

        if (closestDist < rayLength) {
            info.groundHeight = std::max(info.groundHeight, baseY - closestDist);
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
