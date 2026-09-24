// Host stand-in for the Jolt-backed oblivion::PhysicsManager.
//
// The Android build compiles the real manager (physics/physics_manager.cpp) against
// Jolt Physics. That library is far too large to build for a host smoke test, and the
// suites wired into this harness only touch gameplay/save/world logic, so the host
// build links this stand-in instead. It reports "physics unavailable": every entry
// point is a no-op and createCharacter() returns nullptr, which is exactly the
// "physics disabled" path the callers already guard with a null check. Nothing here
// fabricates simulation results, so no suite can silently pass on fake physics.
#include "physics/physics_manager.h"

namespace oblivion {

bool PhysicsManager::init() {
    return false;
}

void PhysicsManager::update(float) {}

void PhysicsManager::shutdown() {}

void PhysicsManager::createTerrainFromLand(const float*, int, float, float, float) {}

JPH::CharacterVirtual* PhysicsManager::createCharacter(const glm::vec3&, float, float) {
    return nullptr;
}

void PhysicsManager::updateCharacter(JPH::CharacterVirtual*, float, const glm::vec3&) {}

glm::vec3 PhysicsManager::getCharacterPosition(JPH::CharacterVirtual*) const {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

void PhysicsManager::setCharacterPosition(JPH::CharacterVirtual*, const glm::vec3&) {}

void PhysicsManager::snapCharacterToGround(JPH::CharacterVirtual*, float) {}

bool PhysicsManager::isCharacterGrounded(JPH::CharacterVirtual*) const {
    return false;
}

void PhysicsManager::destroyCharacter(JPH::CharacterVirtual*) {}

JPH::BodyID PhysicsManager::createBox(const glm::vec3&, const glm::vec3&, float) {
    return JPH::BodyID();
}

JPH::BodyID PhysicsManager::createSphere(const glm::vec3&, float, float) {
    return JPH::BodyID();
}

void PhysicsManager::setBodyPosition(JPH::BodyID, const glm::vec3&) {}

glm::vec3 PhysicsManager::getBodyPosition(JPH::BodyID) const {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

void PhysicsManager::removeBody(JPH::BodyID) {}

bool PhysicsManager::raycast(const Ray&, RaycastHit&) {
    return false;
}

}  // namespace oblivion
