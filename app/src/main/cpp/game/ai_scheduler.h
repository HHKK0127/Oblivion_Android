#pragma once

// ============================================================================
// Phase 35: Radiant AI — Scheduler
// ============================================================================
// The AI Scheduler manages NPC behavior by evaluating AI packages each frame.
// It integrates with the game's time system and drives NPC movement/actions.
//
// Key behaviors:
//   - 24h time-based package selection (sleep at night, work during day)
//   - Priority-based package interruption
//   - Smooth transitions between packages
//   - Integration with NavMesh pathfinding
// ============================================================================

#include "ai_package.h"
#include <unordered_map>
#include <memory>
#include <android/log.h>

#define LOG_TAG "AIScheduler"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations
class NPC;
class NpcManager;
class WorldManager;

namespace oblivion {
    class NavMeshManager;
}

namespace ai {

// ============================================================================
// AI Scheduler
// ============================================================================
class AIScheduler {
public:
    AIScheduler();
    ~AIScheduler();

    // Initialize with game systems
    void init(NpcManager* npcMgr, WorldManager* worldMgr,
              oblivion::NavMeshManager* navMesh);

    // Update all NPC AI (call once per frame)
    void update(float deltaTime);

    // Register an NPC with default packages
    void registerNPC(uint32_t npcId);

    // Register an NPC with custom packages
    void registerNPC(uint32_t npcId, const std::vector<AIPackage>& packages);

    // Unregister an NPC
    void unregisterNPC(uint32_t npcId);

    // Add a package to an NPC
    void addPackage(uint32_t npcId, const AIPackage& pkg);

    // Remove a package from an NPC
    void removePackage(uint32_t npcId, uint32_t packageId);

    // Get the active package for an NPC
    const AIPackage* getActivePackage(uint32_t npcId) const;

    // Get package stack for an NPC
    const PackageStack* getPackageStack(uint32_t npcId) const;

    // Trigger combat for an NPC (interrupts current package)
    void triggerCombat(uint32_t npcId, uint32_t targetId);

    // Trigger flee for an NPC
    void triggerFlee(uint32_t npcId, const glm::vec3& threatPosition);

    // Clear combat (NPC returns to normal behavior)
    void clearCombat(uint32_t npcId);

    // Get current game hour (0.0 - 24.0)
    float getGameHour() const { return gameHour; }

    // Set game hour (for time manipulation)
    void setGameHour(float hour) { gameHour = hour; }

    // Advance time
    void advanceTime(float hours);

    // Statistics
    size_t getRegisteredNPCCount() const { return npcPackages.size(); }

private:
    NpcManager* npcManager = nullptr;
    WorldManager* worldManager = nullptr;
    oblivion::NavMeshManager* navMeshManager = nullptr;

    // Game time (0.0 - 24.0, wraps)
    float gameHour = 8.0f;  // Start at 8:00 AM
    float timeScale = 1.0f; // Real-time to game-time multiplier

    // NPC package stacks
    std::unordered_map<uint32_t, PackageStack> npcPackages;

    // NPC state tracking
    struct NPCAIState {
        glm::vec3 lastPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        float stuckTimer = 0.0f;
        float packageTimer = 0.0f;
        bool movingToTarget = false;
        glm::vec3 moveTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        uint32_t nextPackageId = 1;
    };
    std::unordered_map<uint32_t, NPCAIState> npcStates;

    // Update a single NPC's AI
    void updateNPC(uint32_t npcId, NPC* npc, float deltaTime);

    // Execute a package (move NPC, play animations, etc.)
    void executePackage(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);

    // Execute specific package types
    void executeWander(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeTravel(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executePatrol(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeFollow(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeGuard(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeSleep(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeEat(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeSandBox(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeCombat(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);
    void executeFlee(uint32_t npcId, NPC* npc, AIPackage& pkg, float deltaTime);

    // Move NPC toward a target position
    void moveToward(NPC* npc, const glm::vec3& target, float deltaTime, float speed = -1.0f);

    // Check if NPC has reached a position
    bool hasReached(const NPC* npc, const glm::vec3& target, float threshold = 1.0f) const;

    // Get random position within radius
    glm::vec3 getRandomPosition(const glm::vec3& center, float radius) const;

    // Check if NPC is stuck (hasn't moved)
    bool isStuck(uint32_t npcId, const NPC* npc, float threshold = 0.1f);

    // Create default daily schedule for an NPC
    std::vector<AIPackage> createDefaultSchedule(uint32_t npcId) const;
};

} // namespace ai
