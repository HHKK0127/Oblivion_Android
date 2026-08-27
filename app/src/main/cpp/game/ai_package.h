#pragma once

// ============================================================================
// Phase 35: Radiant AI — Package System
// ============================================================================
// In Oblivion, each NPC has a stack of AI packages that determine their
// behavior. The scheduler evaluates conditions each frame and activates
// the highest-priority eligible package.
//
// Package types mirror original Oblivion:
//   Eat, Sleep, Wander, Travel, Patrol, Follow, Escort, Guard,
//   Activate, Combat, Flee, Accompany, UseItemAt, SandBox
//
// Each package has:
//   - Type-specific data (target, location, duration, etc.)
//   - Conditions (time of day, health %, proximity, etc.)
//   - Priority (higher overrides lower)
// ============================================================================

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

// Forward declarations
class NPC;

namespace ai {

// ============================================================================
// Package Types
// ============================================================================
enum class PackageType : uint8_t {
    NONE = 0,
    EAT,            // Go to food source and eat
    SLEEP,          // Go to bed and sleep
    WANDER,         // Random wandering within radius
    TRAVEL,         // Walk to a specific location
    PATROL,         // Follow a series of waypoints in order
    FOLLOW,         // Follow a target (player or NPC)
    ESCORT,         // Lead a target to a destination
    GUARD,          // Guard a specific area/door
    ACTIVATE,       // Activate a specific object
    COMBAT,         // Engage in combat
    FLEE,           // Run away from threat
    ACCOMPANY,      // Follow and assist (like FOLLOW but with combat assist)
    USE_ITEM,       // Use a specific item
    SAND_BOX,       // Idle behavior: use nearby furniture, wander, etc.
    DIALOGUE,       // Engage in dialogue with target
    COUNT
};

// ============================================================================
// Time-of-Day Conditions
// ============================================================================
struct TimeCondition {
    bool enabled = false;
    uint8_t startHour = 0;    // 0-23
    uint8_t startMinute = 0;  // 0-59
    uint8_t endHour = 24;
    uint8_t endMinute = 0;

    bool isWithinTime(float hourOfDay) const {
        if (!enabled) return true;  // No time restriction
        float start = startHour + startMinute / 60.0f;
        float end = endHour + endMinute / 60.0f;
        if (start <= end) {
            return hourOfDay >= start && hourOfDay < end;
        } else {
            // Wraps midnight (e.g., 22:00 - 06:00)
            return hourOfDay >= start || hourOfDay < end;
        }
    }
};

// ============================================================================
// Location Conditions
// ============================================================================
struct LocationCondition {
    bool enabled = false;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    float radius = 0.0f;          // 0 = exact position
    uint32_t cellFormID = 0;      // 0 = any cell

    bool isWithinLocation(const glm::vec3& npcPos, uint32_t npcCellID) const {
        if (!enabled) return true;
        if (cellFormID != 0 && npcCellID != cellFormID) return false;
        if (radius <= 0.0f) return true;  // No distance check
        float dx = npcPos.x - position.x;
        float dz = npcPos.z - position.z;
        return (dx * dx + dz * dz) <= (radius * radius);
    }
};

// ============================================================================
// Health/Stat Conditions
// ============================================================================
struct StatCondition {
    bool enabled = false;
    float healthPercentMin = 0.0f;   // 0.0 - 1.0
    float healthPercentMax = 1.0f;
    float magickaPercentMin = 0.0f;
    float staminaPercentMin = 0.0f;

    bool isMet(float healthPct, float magickaPct, float staminaPct) const {
        if (!enabled) return true;
        return healthPct >= healthPercentMin && healthPct <= healthPercentMax
            && magickaPct >= magickaPercentMin
            && staminaPct >= staminaPercentMin;
    }
};

// ============================================================================
// Package Conditions (combined)
// ============================================================================
struct PackageConditions {
    TimeCondition time;
    LocationCondition location;
    StatCondition stat;

    // Evaluate all conditions
    bool evaluate(float hourOfDay, const glm::vec3& npcPos,
                  uint32_t npcCellID,
                  float healthPct, float magickaPct, float staminaPct) const {
        return time.isWithinTime(hourOfDay)
            && location.isWithinLocation(npcPos, npcCellID)
            && stat.isMet(healthPct, magickaPct, staminaPct);
    }
};

// ============================================================================
// Package Data — type-specific parameters
// ============================================================================
struct PackageData {
    // Common
    float duration = -1.0f;       // -1 = indefinite, >0 = seconds
    bool interruptible = true;    // Can be interrupted by higher priority

    // Wander
    float wanderRadius = 5.0f;    // Wander radius from origin
    uint8_t wanderDirection = 0;  // 0=random, 1=N, 2=E, 3=S, 4=W

    // Travel / Patrol
    glm::vec3 destination = glm::vec3(0.0f, 0.0f, 0.0f);
    std::vector<glm::vec3> patrolWaypoints;
    bool patrolLoop = true;       // Loop back to start

    // Follow / Escort / Accompany
    uint32_t targetId = 0;        // NPC ID to follow/escort
    float followDistance = 3.0f;  // Desired distance to target
    float followMaxDistance = 50.0f; // Give up if too far

    // Guard
    glm::vec3 guardPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    float guardRadius = 10.0f;

    // Activate
    uint32_t activateObjectId = 0; // Object FormID to activate

    // Eat / Sleep
    glm::vec3 bedPosition = glm::vec3(0.0f, 0.0f, 0.0f); // For sleep
    float sleepDuration = 8.0f;   // Hours
    float eatDuration = 1.0f;     // Hours

    // Combat
    uint32_t combatTargetId = 0;

    // Flee
    glm::vec3 fleeFrom = glm::vec3(0.0f, 0.0f, 0.0f);
    float fleeDistance = 50.0f;

    // SandBox
    float sandboxRadius = 15.0f;
    std::vector<uint32_t> furnitureIds; // Furniture to use
};

// ============================================================================
// AI Package
// ============================================================================
struct AIPackage {
    uint32_t packageId = 0;       // Unique ID
    PackageType type = PackageType::NONE;
    uint8_t priority = 50;        // 0-100, higher = more important
    PackageConditions conditions;
    PackageData data;

    // State tracking
    bool active = false;
    float elapsed = 0.0f;         // Time since package started
    uint32_t currentPatrolIndex = 0; // For patrol packages

    // Reset state
    void reset() {
        active = false;
        elapsed = 0.0f;
        currentPatrolIndex = 0;
    }
};

// ============================================================================
// Package Stack — manages active and queued packages
// ============================================================================
class PackageStack {
public:
    PackageStack() = default;

    // Add a package to the stack (higher priority inserts above lower)
    void pushPackage(const AIPackage& pkg);

    // Remove a package by ID
    void removePackage(uint32_t packageId);

    // Remove all packages of a type
    void removePackagesByType(PackageType type);

    // Get the currently active (highest priority eligible) package
    AIPackage* getActivePackage();

    // Evaluate conditions and update active package
    void evaluate(float hourOfDay, const glm::vec3& npcPos,
                  uint32_t npcCellID,
                  float healthPct, float magickaPct, float staminaPct);

    // Clear all packages
    void clear() { packages.clear(); }

    // Get all packages
    const std::vector<AIPackage>& getPackages() const { return packages; }

    // Size
    size_t size() const { return packages.size(); }
    bool empty() const { return packages.empty(); }

private:
    std::vector<AIPackage> packages;  // Sorted by priority (highest first)
};

// ============================================================================
// Package Factory — create common package configurations
// ============================================================================
namespace PackageFactory {

    inline AIPackage createWander(uint32_t id, float radius, uint8_t priority = 30) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::WANDER;
        pkg.priority = priority;
        pkg.data.wanderRadius = radius;
        return pkg;
    }

    inline AIPackage createTravel(uint32_t id, const glm::vec3& dest, uint8_t priority = 40) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::TRAVEL;
        pkg.priority = priority;
        pkg.data.destination = dest;
        return pkg;
    }

    inline AIPackage createPatrol(uint32_t id, const std::vector<glm::vec3>& waypoints,
                                   bool loop = true, uint8_t priority = 40) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::PATROL;
        pkg.priority = priority;
        pkg.data.patrolWaypoints = waypoints;
        pkg.data.patrolLoop = loop;
        return pkg;
    }

    inline AIPackage createFollow(uint32_t id, uint32_t targetId, float distance = 3.0f,
                                   uint8_t priority = 50) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::FOLLOW;
        pkg.priority = priority;
        pkg.data.targetId = targetId;
        pkg.data.followDistance = distance;
        return pkg;
    }

    inline AIPackage createSleep(uint32_t id, const glm::vec3& bedPos,
                                  uint8_t startHour = 22, uint8_t endHour = 6,
                                  uint8_t priority = 60) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::SLEEP;
        pkg.priority = priority;
        pkg.data.bedPosition = bedPos;
        pkg.conditions.time.enabled = true;
        pkg.conditions.time.startHour = startHour;
        pkg.conditions.time.endHour = endHour;
        return pkg;
    }

    inline AIPackage createEat(uint32_t id, const glm::vec3& foodPos,
                                uint8_t startHour = 12, uint8_t endHour = 13,
                                uint8_t priority = 55) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::EAT;
        pkg.priority = priority;
        pkg.data.destination = foodPos;
        pkg.conditions.time.enabled = true;
        pkg.conditions.time.startHour = startHour;
        pkg.conditions.time.endHour = endHour;
        return pkg;
    }

    inline AIPackage createGuard(uint32_t id, const glm::vec3& pos, float radius = 10.0f,
                                  uint8_t priority = 45) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::GUARD;
        pkg.priority = priority;
        pkg.data.guardPosition = pos;
        pkg.data.guardRadius = radius;
        return pkg;
    }

    inline AIPackage createSandBox(uint32_t id, float radius = 15.0f, uint8_t priority = 20) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::SAND_BOX;
        pkg.priority = priority;
        pkg.data.sandboxRadius = radius;
        return pkg;
    }

    inline AIPackage createCombat(uint32_t id, uint32_t targetId, uint8_t priority = 90) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::COMBAT;
        pkg.priority = priority;
        pkg.data.combatTargetId = targetId;
        pkg.data.interruptible = false;
        return pkg;
    }

    inline AIPackage createFlee(uint32_t id, const glm::vec3& from, float distance = 50.0f,
                                 uint8_t priority = 95) {
        AIPackage pkg;
        pkg.packageId = id;
        pkg.type = PackageType::FLEE;
        pkg.priority = priority;
        pkg.data.fleeFrom = from;
        pkg.data.fleeDistance = distance;
        pkg.data.interruptible = false;
        return pkg;
    }

} // namespace PackageFactory

} // namespace ai
