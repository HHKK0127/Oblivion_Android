// ============================================================================
// Phase 35: Radiant AI — Scheduler Implementation
// ============================================================================

#include "ai_scheduler.h"
#include "npc.h"
#include "npc_manager.h"
#include "../world/world_manager.h"
#include "navmesh_manager.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace ai {

// Seconds an NPC waits after losing a too-far follow target before it may
// retry following. Prevents an immediate reset/restart loop every frame.
static constexpr float kFOLLOW_GIVEUP_TIME = 10.0f;

AIScheduler::AIScheduler() {
    LOGD("AIScheduler created");
}

AIScheduler::~AIScheduler() {
    LOGD("AIScheduler destroyed");
}

void AIScheduler::init(NpcManager* npcMgr, WorldManager* worldMgr,
                        oblivion::NavMeshManager* navMesh) {
    npcManager = npcMgr;
    worldManager = worldMgr;
    navMeshManager = navMesh;
    LOGI("AIScheduler initialized");
}

// ============================================================================
// NPC Registration
// ============================================================================

void AIScheduler::registerNPC(uint32_t npcId) {
    auto packages = createDefaultSchedule(npcId);
    registerNPC(npcId, packages);
}

void AIScheduler::registerNPC(uint32_t npcId, const std::vector<AIPackage>& packages) {
    PackageStack& stack = npcPackages[npcId];
    stack.clear();
    for (const auto& pkg : packages) {
        stack.pushPackage(pkg);
    }

    NPCAIState& state = npcStates[npcId];
        state.nextPackageId = packages.size() + 1;
    }

void AIScheduler::unregisterNPC(uint32_t npcId) {
    npcPackages.erase(npcId);
    npcStates.erase(npcId);
    LOGD("Unregistered NPC %u", npcId);
}

// ============================================================================
// Package Management
// ============================================================================

void AIScheduler::addPackage(uint32_t npcId, const AIPackage& pkg) {
    auto it = npcPackages.find(npcId);
    if (it == npcPackages.end()) {
        LOGW("Cannot add package to unregistered NPC %u", npcId);
        return;
    }
    it->second.pushPackage(pkg);
    LOGD("Added package %u (type %d) to NPC %u", pkg.packageId,
         static_cast<int>(pkg.type), npcId);
}

void AIScheduler::removePackage(uint32_t npcId, uint32_t packageId) {
    auto it = npcPackages.find(npcId);
    if (it != npcPackages.end()) {
        it->second.removePackage(packageId);
    }
}

const AIPackage* AIScheduler::getActivePackage(uint32_t npcId) const {
    auto it = npcPackages.find(npcId);
    if (it != npcPackages.end()) {
        return const_cast<PackageStack&>(it->second).getActivePackage();
    }
    return nullptr;
}

const PackageStack* AIScheduler::getPackageStack(uint32_t npcId) const {
    auto it = npcPackages.find(npcId);
    if (it != npcPackages.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Combat / Flee Triggers
// ============================================================================

void AIScheduler::triggerCombat(uint32_t npcId, uint32_t targetId) {
    auto it = npcPackages.find(npcId);
    if (it == npcPackages.end()) return;

    // Remove existing combat/flee packages
    it->second.removePackagesByType(PackageType::COMBAT);
    it->second.removePackagesByType(PackageType::FLEE);

    // Add combat package at high priority
    AIPackage combat = PackageFactory::createCombat(npcStates[npcId].nextPackageId++, targetId);
    it->second.pushPackage(combat);
    }

void AIScheduler::triggerFlee(uint32_t npcId, const glm::vec3& threatPosition) {
    auto it = npcPackages.find(npcId);
    if (it == npcPackages.end()) return;

    // Remove combat packages
    it->second.removePackagesByType(PackageType::COMBAT);

    // Add flee package at highest priority
    AIPackage flee = PackageFactory::createFlee(npcStates[npcId].nextPackageId++,
                                                 threatPosition);
    it->second.pushPackage(flee);

    LOGD("NPC %u fleeing from threat at (%.1f, %.1f, %.1f)",
         npcId, threatPosition.x, threatPosition.y, threatPosition.z);
}

void AIScheduler::clearCombat(uint32_t npcId) {
    auto it = npcPackages.find(npcId);
    if (it == npcPackages.end()) return;

    it->second.removePackagesByType(PackageType::COMBAT);
    it->second.removePackagesByType(PackageType::FLEE);

    LOGD("NPC %u cleared combat packages", npcId);
}

// ============================================================================
// Main Update Loop
// ============================================================================

void AIScheduler::update(float deltaTime) {
    if (!npcManager) return;

    // Advance game time
    // In Oblivion, 1 real minute = 30 game minutes (timeScale = 30)
    // So 1 real second = 0.5 game minutes = 1/120 game hours
    constexpr float DEFAULT_TIME_SCALE = 30.0f; // 30x speed
    gameHour += (deltaTime * DEFAULT_TIME_SCALE) / 3600.0f;
    while (gameHour >= 24.0f) gameHour -= 24.0f;
    while (gameHour < 0.0f) gameHour += 24.0f;

    // Update each registered NPC
    for (auto& [npcId, stack] : npcPackages) {
        auto npc = npcManager->getNPC(npcId);
        if (!npc || !npc->status.isAlive()) continue;

        updateNPC(npcId, npc.get(), deltaTime);
    }
}

void AIScheduler::updateNPC(uint32_t npcId, NPC* npc, float deltaTime) {
    auto stateIt = npcStates.find(npcId);
    if (stateIt == npcStates.end()) return;
    NPCAIState& state = stateIt->second;

    auto stackIt = npcPackages.find(npcId);
    if (stackIt == npcPackages.end()) return;
    PackageStack& stack = stackIt->second;

    // Calculate NPC stats for condition evaluation
    float healthPct = npc->status.currentHealth / std::max(npc->status.maxHealth, 1.0f);
    float magickaPct = npc->status.currentMana / std::max(npc->status.maxMana, 1.0f);
    float staminaPct = npc->status.stamina / std::max(npc->status.maxStamina, 1.0f);

    // Evaluate package conditions
    stack.evaluate(gameHour, npc->position, 0, healthPct, magickaPct, staminaPct);

    // Get active package
    AIPackage* activePkg = stack.getActivePackage();
    if (!activePkg) {
        // No eligible package — idle
        npc->setAIState(AIState::IDLE);
        return;
    }

    // Check if package changed
    if (!activePkg->active) {
        activePkg->active = true;
        activePkg->elapsed = 0.0f;
        state.stuckTimer = 0.0f;
        state.movingToTarget = false;
            }

    // Update elapsed time
    activePkg->elapsed += deltaTime;
    state.packageTimer += deltaTime;

    // Check duration
    if (activePkg->data.duration > 0.0f && activePkg->elapsed >= activePkg->data.duration) {
        LOGD("NPC %u package %u duration expired", npcId, activePkg->packageId);
        activePkg->reset();
        return;
    }

    // Execute the package
    executePackage(npcId, npc, *activePkg, deltaTime);
}

// ============================================================================
// Package Execution
// ============================================================================

void AIScheduler::executePackage(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    switch (pkg.type) {
        case PackageType::WANDER:   executeWander(npcId, npc, pkg, deltaTime); break;
        case PackageType::TRAVEL:   executeTravel(npcId, npc, pkg, deltaTime); break;
        case PackageType::PATROL:   executePatrol(npcId, npc, pkg, deltaTime); break;
        case PackageType::FOLLOW:   executeFollow(npcId, npc, pkg, deltaTime); break;
        case PackageType::ESCORT:   executeEscort(npcId, npc, pkg, deltaTime); break;
        case PackageType::GUARD:    executeGuard(npcId, npc, pkg, deltaTime); break;
        case PackageType::SLEEP:    executeSleep(npcId, npc, pkg, deltaTime); break;
        case PackageType::EAT:      executeEat(npcId, npc, pkg, deltaTime); break;
        case PackageType::SAND_BOX: executeSandBox(npcId, npc, pkg, deltaTime); break;
        case PackageType::COMBAT:   executeCombat(npcId, npc, pkg, deltaTime); break;
        case PackageType::FLEE:     executeFlee(npcId, npc, pkg, deltaTime); break;
        default:
            // Unsupported package type — just idle
            npc->setAIState(AIState::IDLE);
            break;
    }
}

void AIScheduler::executeWander(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    auto stateIt = npcStates.find(npcId);
    if (stateIt == npcStates.end()) return;
    NPCAIState& state = stateIt->second;

    npc->setAIState(AIState::WANDER);

    // If not moving or reached target, pick a new random position
    if (!state.movingToTarget || hasReached(npc, state.moveTarget)) {
        // Pick new wander target
        glm::vec3 origin = npc->position;  // Could use package origin instead
        state.moveTarget = getRandomPosition(origin, pkg.data.wanderRadius);
        state.movingToTarget = true;
        state.stuckTimer = 0.0f;

        // Use NavMesh pathfinding if available
        if (navMeshManager) {
            std::vector<glm::vec3> path;
            if (navMeshManager->findPath(npc->position, state.moveTarget, path)) {
                npc->currentPath = path;
                npc->currentPathIndex = 0;
            }
        }
    }

    // Move toward target
    if (!npc->currentPath.empty() && npc->currentPathIndex < static_cast<int>(npc->currentPath.size())) {
        // Follow NavMesh path
        glm::vec3 waypoint = npc->currentPath[npc->currentPathIndex];
        moveToward(npc, waypoint, deltaTime);
        if (hasReached(npc, waypoint, 0.5f)) {
            npc->currentPathIndex++;
            if (npc->currentPathIndex >= static_cast<int>(npc->currentPath.size())) {
                npc->currentPath.clear();
                npc->currentPathIndex = 0;
                state.movingToTarget = false;
            }
        }
    } else {
        // Direct movement
        moveToward(npc, state.moveTarget, deltaTime);
    }

    // Check if stuck
    if (isStuck(npcId, npc)) {
        state.movingToTarget = false;  // Pick new target next frame
    }
}

void AIScheduler::executeTravel(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    auto stateIt = npcStates.find(npcId);
    if (stateIt == npcStates.end()) return;
    NPCAIState& state = stateIt->second;

    npc->setAIState(AIState::WANDER);  // Use WANDER state for movement

    if (hasReached(npc, pkg.data.destination)) {
        // Reached destination
        pkg.reset();
        npc->setAIState(AIState::IDLE);
        LOGD("NPC %u reached travel destination", npcId);
        return;
    }

    // Initialize path if needed
    if (!state.movingToTarget) {
        state.moveTarget = pkg.data.destination;
        state.movingToTarget = true;

        if (navMeshManager) {
            std::vector<glm::vec3> path;
            if (navMeshManager->findPath(npc->position, pkg.data.destination, path)) {
                npc->currentPath = path;
                npc->currentPathIndex = 0;
            }
        }
    }

    // Move along path
    if (!npc->currentPath.empty() && npc->currentPathIndex < static_cast<int>(npc->currentPath.size())) {
        glm::vec3 waypoint = npc->currentPath[npc->currentPathIndex];
        moveToward(npc, waypoint, deltaTime);
        if (hasReached(npc, waypoint, 0.5f)) {
            npc->currentPathIndex++;
        }
    } else {
        moveToward(npc, pkg.data.destination, deltaTime);
    }
}

void AIScheduler::executePatrol(uint32_t /*npcId*/, NPC* npc, AIPackage& pkg, float deltaTime) {
    if (pkg.data.patrolWaypoints.empty()) {
        npc->setAIState(AIState::IDLE);
        return;
    }

    npc->setAIState(AIState::PATROL);

    glm::vec3 currentWaypoint = pkg.data.patrolWaypoints[pkg.currentPatrolIndex];

    if (hasReached(npc, currentWaypoint, 1.0f)) {
        // Move to next waypoint
        pkg.currentPatrolIndex++;
        if (pkg.currentPatrolIndex >= pkg.data.patrolWaypoints.size()) {
            if (pkg.data.patrolLoop) {
                pkg.currentPatrolIndex = 0;
            } else {
                pkg.reset();
                npc->setAIState(AIState::IDLE);
                return;
            }
        }
    }

    moveToward(npc, currentWaypoint, deltaTime);
}

void AIScheduler::executeFollow(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    auto stateIt = npcStates.find(npcId);
    if (stateIt == npcStates.end()) return;
    NPCAIState& state = stateIt->second;

    // While in a give-up cooldown, stay idle and do NOT reset the package.
    // Without this, resetting every frame makes the scheduler re-select the
    // same package and spam "started/lost follow" logs in an infinite loop.
    if (state.giveUpTimer > 0.0f) {
        state.giveUpTimer = std::max(0.0f, state.giveUpTimer - deltaTime);
        npc->setAIState(AIState::IDLE);
        return;
    }

    // targetId 0 is the player (no NPC owns formID 0); fall back to the NPC
    // manager only for real actor targets.
    glm::vec3 targetPos;
    bool haveTarget = false;

    if (pkg.data.targetId == 0) {
        if (worldManager) {
            targetPos = worldManager->getPlayerPosition();
            haveTarget = true;
        }
    } else if (npcManager) {
        auto target = npcManager->getNPC(pkg.data.targetId);
        if (target) {
            targetPos = target->position;
            haveTarget = true;
        }
    }

    if (!haveTarget) {
        npc->setAIState(AIState::IDLE);
        return;
    }

    npc->setAIState(AIState::FOLLOW_PLAYER);

    float distSq = 0.0f;
    {
        float dx = npc->position.x - targetPos.x;
        float dz = npc->position.z - targetPos.z;
        distSq = dx * dx + dz * dz;
    }

    float followDist = pkg.data.followDistance;
    float maxDist = pkg.data.followMaxDistance;

    if (distSq > maxDist * maxDist) {
            // Target is too far away: enter a give-up cooldown instead of resetting
            // the package. Resetting here made the scheduler re-select and re-look
            // the same FOLLOW package every frame (infinite loop).
            state.giveUpTimer = kFOLLOW_GIVEUP_TIME;
                        npc->setAIState(AIState::IDLE);
            return;
        }

    if (distSq > followDist * followDist) {
        // Too far — move closer
        moveToward(npc, targetPos, deltaTime);
    } else {
        // Close enough — stop
        npc->setAIState(AIState::IDLE);
    }
}

void AIScheduler::executeEscort(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    // Escort leads an actor (targetId 0 = player) toward destination and keeps
    // a followDistance gap, slowing down or waiting whenever the escorted
    // actor falls behind beyond followMaxDistance.
    glm::vec3 escortPos;
    bool haveEscortee = false;

    if (pkg.data.targetId == 0) {
        if (worldManager) {
            escortPos = worldManager->getPlayerPosition();
            haveEscortee = true;
        }
    } else if (npcManager) {
        auto target = npcManager->getNPC(pkg.data.targetId);
        if (target) {
            escortPos = target->position;
            haveEscortee = true;
        }
    }

    if (!haveEscortee) {
        LOGD("NPC %u escort target %u not found", npcId, pkg.data.targetId);
        npc->setAIState(AIState::IDLE);
        return;
    }

    npc->setAIState(AIState::FOLLOW_PLAYER);

    float gap = 0.0f;
    {
        float dx = npc->position.x - escortPos.x;
        float dz = npc->position.z - escortPos.z;
        gap = std::sqrt(dx * dx + dz * dz);
    }

    // Escorted actor strayed too far: hold position until they catch up.
    if (gap > pkg.data.followMaxDistance) {
        npc->setAIState(AIState::IDLE);
        return;
    }

    // Destination reached (both the NPC and the escorted actor are close):
    // stop leading and let the package restart on the next selection.
    bool selfAtDestination = hasReached(npc, pkg.data.destination, 2.0f);
    bool escorteeAtDestination = false;
    {
        float dx = escortPos.x - pkg.data.destination.x;
        float dz = escortPos.z - pkg.data.destination.z;
        escorteeAtDestination =
            (dx * dx + dz * dz) <= (pkg.data.followMaxDistance * pkg.data.followMaxDistance);
    }

    if (selfAtDestination && escorteeAtDestination) {
        LOGD("NPC %u finished escort to destination", npcId);
        pkg.reset();
        npc->setAIState(AIState::IDLE);
        return;
    }

    // Wait when the escorted actor is closer to us than followDistance; keep
    // walking toward the destination otherwise.
    if (gap > pkg.data.followDistance && escorteeAtDestination) {
        npc->setAIState(AIState::IDLE);
        return;
    }

    float moveSpeed = npc->moveSpeed;
    // Do not outrun the escorted actor.
    if (gap < pkg.data.followDistance) {
        moveSpeed *= 0.5f;
    }

    moveToward(npc, pkg.data.destination, deltaTime, moveSpeed);
}

void AIScheduler::executeGuard(uint32_t /*npcId*/, NPC* npc, AIPackage& pkg, float deltaTime) {
    npc->setAIState(AIState::PATROL);  // Use PATROL for guard behavior

    float dx = npc->position.x - pkg.data.guardPosition.x;
    float dz = npc->position.z - pkg.data.guardPosition.z;
    float distSq = dx * dx + dz * dz;

    if (distSq > pkg.data.guardRadius * pkg.data.guardRadius) {
        // Return to guard post
        moveToward(npc, pkg.data.guardPosition, deltaTime);
    } else {
        // At guard post — idle (could add look-around behavior)
        npc->setAIState(AIState::IDLE);
    }
}

void AIScheduler::executeSleep(uint32_t /*npcId*/, NPC* npc, AIPackage& pkg, float deltaTime) {
    // Move to bed if not there
    if (!hasReached(npc, pkg.data.bedPosition, 2.0f)) {
        npc->setAIState(AIState::WANDER);
        moveToward(npc, pkg.data.bedPosition, deltaTime);
    } else {
        // Sleeping — regenerate health/magicka
        npc->setAIState(AIState::IDLE);
        npc->status.currentHealth = std::min(npc->status.currentHealth +
            npc->status.maxHealth * 0.05f * deltaTime, npc->status.maxHealth);
        npc->status.currentMana = std::min(npc->status.currentMana +
            npc->status.maxMana * 0.1f * deltaTime, npc->status.maxMana);
    }
}

void AIScheduler::executeEat(uint32_t /*npcId*/, NPC* npc, AIPackage& pkg, float deltaTime) {
    // Move to food location
    if (!hasReached(npc, pkg.data.destination, 2.0f)) {
        npc->setAIState(AIState::WANDER);
        moveToward(npc, pkg.data.destination, deltaTime);
    } else {
        // Eating — regenerate health
        npc->setAIState(AIState::IDLE);
        npc->status.currentHealth = std::min(npc->status.currentHealth +
            npc->status.maxHealth * 0.02f * deltaTime, npc->status.maxHealth);
    }
}

void AIScheduler::executeSandBox(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    // Sandbox = wander + idle randomly
    auto stateIt = npcStates.find(npcId);
    if (stateIt == npcStates.end()) return;
    NPCAIState& state = stateIt->second;

    // 70% chance to wander, 30% idle
    if (!state.movingToTarget) {
        if ((std::rand() % 100) < 70) {
            state.moveTarget = getRandomPosition(npc->position, pkg.data.sandboxRadius);
            state.movingToTarget = true;
        } else {
            npc->setAIState(AIState::IDLE);
            return;
        }
    }

    npc->setAIState(AIState::WANDER);
    moveToward(npc, state.moveTarget, deltaTime);

    if (hasReached(npc, state.moveTarget)) {
        state.movingToTarget = false;
    }
}

void AIScheduler::executeCombat(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    if (!npcManager) {
        pkg.reset();
        return;
    }

    auto target = npcManager->getNPC(pkg.data.combatTargetId);
    if (!target || !target->status.isAlive()) {
        clearCombat(npcId);
        return;
    }

    npc->setAIState(AIState::COMBAT);

    // Move toward target if too far
    float dx = npc->position.x - target->position.x;
    float dz = npc->position.z - target->position.z;
    float distSq = dx * dx + dz * dz;

    constexpr float ATTACK_RANGE = 3.0f;
    if (distSq > ATTACK_RANGE * ATTACK_RANGE) {
        moveToward(npc, target->position, deltaTime, npc->moveSpeed * 1.2f);
    }
    // Actual combat damage is handled by CombatManager
}

void AIScheduler::executeFlee(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime) {
    npc->setAIState(AIState::WANDER);  // Use WANDER for fleeing

    // Calculate flee direction (away from threat)
    glm::vec3 fleeDir = npc->position - pkg.data.fleeFrom;
    float len = std::sqrt(fleeDir.x * fleeDir.x + fleeDir.z * fleeDir.z);
    if (len > 0.01f) {
        fleeDir.x /= len;
        fleeDir.z /= len;
    } else {
        // Random direction if at threat position
        float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 6.2831853f;
        fleeDir.x = std::cos(angle);
        fleeDir.z = std::sin(angle);
    }

    // Move away from threat
    glm::vec3 fleeTarget;
    fleeTarget.x = npc->position.x + fleeDir.x * pkg.data.fleeDistance;
    fleeTarget.y = npc->position.y;
    fleeTarget.z = npc->position.z + fleeDir.z * pkg.data.fleeDistance;

    moveToward(npc, fleeTarget, deltaTime, npc->moveSpeed * 1.5f);

    // Check if far enough from threat
    float distFromThreat = 0.0f;
    {
        float dx2 = npc->position.x - pkg.data.fleeFrom.x;
        float dz2 = npc->position.z - pkg.data.fleeFrom.z;
        distFromThreat = std::sqrt(dx2 * dx2 + dz2 * dz2);
    }

    if (distFromThreat >= pkg.data.fleeDistance * 0.8f) {
        // Safe — clear flee
        clearCombat(npcId);
        LOGD("NPC %u reached safety", npcId);
    }
}

// ============================================================================
// Movement Helpers
// ============================================================================

void AIScheduler::moveToward(NPC* npc, const glm::vec3& target, float deltaTime, float speed) {
    if (speed < 0.0f) speed = npc->moveSpeed;

    glm::vec3 direction = target - npc->position;
    float distSq = direction.x * direction.x + direction.z * direction.z;

    if (distSq < 0.01f) return;  // Already there

    float dist = std::sqrt(distSq);
    direction.x /= dist;
    direction.z /= dist;

    float moveAmount = speed * deltaTime;
    if (moveAmount > dist) moveAmount = dist;

    npc->position.x += direction.x * moveAmount;
    npc->position.z += direction.z * moveAmount;

    // Face movement direction
    npc->rotation.y = std::atan2(direction.x, direction.z) * (180.0f / 3.14159265f);
}

bool AIScheduler::hasReached(const NPC* npc, const glm::vec3& target, float threshold) const {
    float dx = npc->position.x - target.x;
    float dz = npc->position.z - target.z;
    return (dx * dx + dz * dz) <= (threshold * threshold);
}

glm::vec3 AIScheduler::getRandomPosition(const glm::vec3& center, float radius) const {
    float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 6.2831853f;
    float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * radius;
    return glm::vec3(
        center.x + std::cos(angle) * r,
        center.y,
        center.z + std::sin(angle) * r
    );
}

bool AIScheduler::isStuck(uint32_t npcId, const NPC* npc, float threshold) {
    auto stateIt = npcStates.find(npcId);
    if (stateIt == npcStates.end()) return false;
    NPCAIState& state = stateIt->second;

    float dx = npc->position.x - state.lastPosition.x;
    float dz = npc->position.z - state.lastPosition.z;
    float moved = dx * dx + dz * dz;

    state.lastPosition = npc->position;

    if (moved < threshold * threshold) {
        state.stuckTimer += 1.0f;  // Approximate
        return state.stuckTimer > 3.0f;  // Stuck for 3 seconds
    } else {
        state.stuckTimer = 0.0f;
        return false;
    }
}

// ============================================================================
// Default Schedule
// ============================================================================

std::vector<AIPackage> AIScheduler::createDefaultSchedule(uint32_t /*npcId*/) const {
    std::vector<AIPackage> packages;
    uint32_t pkgId = 1;

    // Sleep: 22:00 - 06:00 (high priority)
    {
        AIPackage sleep = PackageFactory::createSleep(pkgId++, glm::vec3(0.0f, 0.0f, 0.0f));
        sleep.priority = 60;
        packages.push_back(sleep);
    }

    // Eat: 12:00 - 13:00 (medium priority)
    {
        AIPackage eat = PackageFactory::createEat(pkgId++, glm::vec3(0.0f, 0.0f, 0.0f));
        eat.priority = 55;
        packages.push_back(eat);
    }

    // Wander: all day (low priority)
    {
        AIPackage wander = PackageFactory::createWander(pkgId++, 10.0f, 30);
        packages.push_back(wander);
    }

    // SandBox: fallback (lowest priority)
    {
        AIPackage sandbox = PackageFactory::createSandBox(pkgId++, 15.0f, 20);
        packages.push_back(sandbox);
    }

    return packages;
}

} // namespace ai
