#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <glm/glm.hpp>
#include "npc.h"
#include "quest.h"
#include "spell.h"

// ============================================================================
// Player Data (Core Game State)
// ============================================================================

struct PlayerState {
    // Position & Orientation
    glm::vec3 position;
    glm::vec3 rotation;
    uint32_t currentCell;

    // Character Status
    CharacterStatus status;
    uint32_t playerLevel;
    float experiencePoints;

    // Inventory (60 slots max)
    std::vector<std::pair<uint32_t, uint32_t>> inventory;  // <itemId, quantity>
    float currentWeight;

    // Equipment
    uint32_t equippedWeapon;
    uint32_t equippedSpell;

    PlayerState()
        : position(glm::vec3(0.0f)),
          rotation(glm::vec3(0.0f)),
          currentCell(0),
          playerLevel(1),
          experiencePoints(0.0f),
          currentWeight(0.0f),
          equippedWeapon(0),
          equippedSpell(0) {}
};

// ============================================================================
// World State (Environmental Data)
// ============================================================================

struct WorldState {
    // Time System
    float timeOfDay;        // 0.0-24.0 hours
    int dayCount;           // Days elapsed

    // Weather System
    std::string currentWeather;  // "clear", "rain", "snow", "storm"
    float weatherIntensity;  // 0.0-1.0
    float weatherTransitionTime;

    // Loaded Cells (for efficient reloading)
    std::vector<uint32_t> loadedCells;

    // World Items (dropped items on ground)
    std::vector<std::tuple<uint32_t, glm::vec3>> worldItems;  // <itemId, position>

    // Player Position (for world manager tracking)
    glm::vec3 playerPosition;
    glm::vec3 playerRotation;

    WorldState()
        : timeOfDay(12.0f),
          dayCount(0),
          currentWeather("clear"),
          weatherIntensity(0.0f),
          weatherTransitionTime(0.0f),
          playerPosition(0.0f, 0.0f, 0.0f),
          playerRotation(0.0f, 0.0f, 0.0f) {}
};

// ============================================================================
// NPC State (Individual NPC Data)
// ============================================================================

struct NPCState {
    uint32_t npcId;
    glm::vec3 position;
    glm::vec3 rotation;
    CharacterStatus status;
    AIState aiState;
    float wanderRadius;
    uint32_t currentCell;
    std::vector<uint32_t> availableQuests;
    std::vector<uint32_t> givenQuests;

    NPCState()
        : npcId(0),
          position(glm::vec3(0.0f)),
          rotation(glm::vec3(0.0f)),
          aiState(AIState::IDLE),
          wanderRadius(50.0f),
          currentCell(0) {}
};

// ============================================================================
// Quest State (Individual Quest Progress)
// ============================================================================

struct QuestProgressState {
    uint32_t questId;
    uint32_t giverNpcId;
    QuestState state;
    std::vector<std::pair<uint32_t, uint32_t>> objectiveProgress;  // <objectiveId, currentProgress>
    uint32_t timeAccepted;
    uint32_t timeCompleted;

    QuestProgressState()
        : questId(0),
          giverNpcId(0),
          state(QuestState::PENDING),
          timeAccepted(0),
          timeCompleted(0) {}
};

// ============================================================================
// MOD Support Structures (Optional Fields)
// ============================================================================

// CM Partners MOD - Companion NPC State
struct CompanionState {
    uint32_t companionNpcId;
    std::string companionName;
    bool isActive;
    float relationship;     // 0-100
    uint32_t joinedTime;

    CompanionState()
        : companionNpcId(0),
          isActive(false),
          relationship(0.0f),
          joinedTime(0) {}
};

// Pet MOD - Pet Ownership (Maigrets House Cats, More Female Servants)
struct PetState {
    uint32_t petNpcId;
    std::string petName;
    std::string petType;    // "cat", "dog", "familiar"
    bool isActive;
    glm::vec3 lastKnownPosition;

    PetState()
        : petNpcId(0),
          isActive(false),
          lastKnownPosition(glm::vec3(0.0f)) {}
};

// CSR (Complete Speechcraft Redesign) - NPC Relationship Tracking
struct NPCRelationshipState {
    uint32_t npcId;
    float dispositionValue;  // 0-100
    std::vector<std::string> conversationTopics;  // Topics discussed
    bool hasBeenGreeted;

    NPCRelationshipState()
        : npcId(0),
          dispositionValue(0.0f),
          hasBeenGreeted(false) {}
};

// Carry Capacity MOD - Game Balance
struct GameBalanceState {
    float carryCapacityMultiplier;  // 1.0 = default, 2.0 = 2x capacity
    float damageMultiplier;         // 1.0 = default
    float healthRegenRate;          // Multiplier for health regeneration

    GameBalanceState()
        : carryCapacityMultiplier(1.0f),
          damageMultiplier(1.0f),
          healthRegenRate(1.0f) {}
};

// ============================================================================
// Complete Game State (All Persistent Data)
// ============================================================================

struct GameState {
    // Metadata
    std::string saveName;
    std::string saveTimestamp;  // ISO 8601 format
    std::string version;        // "0.6.0"
    uint32_t checksum;          // CRC32 for integrity validation

    // Core Player & World Data
    PlayerState playerState;
    WorldState worldState;

    // NPC & Quest States
    std::map<uint32_t, NPCState> npcStates;
    std::map<uint32_t, QuestProgressState> questStates;

    // ========== OPTIONAL MOD SUPPORT FIELDS ==========

    // MOD Metadata
    std::vector<std::string> activeMods;  // Which MODs are active
    std::map<std::string, std::string> modVersions;  // MOD version tracking

    // CM Partners MOD - Companion Management
    std::vector<CompanionState> companionStates;

    // Pet MODs (Maigrets House Cats, More Female Servants) - Pet Management
    std::vector<PetState> petStates;
    float petHappiness;  // Overall pet system happiness (0-100)

    // MadCompanionshipSpells & Carry Capacity MOD - Learned MOD Spells
    std::vector<uint32_t> learnedModSpells;  // MOD-added spell IDs

    // CSR (Complete Speechcraft Redesign) - NPC Relationships
    std::vector<NPCRelationshipState> npcRelationships;

    // Game Balance Modifiers
    GameBalanceState gameBalance;

    // ========== END MOD SUPPORT FIELDS ==========

    GameState()
        : saveName(""),
          saveTimestamp(""),
          version("0.6.0"),
          checksum(0),
          petHappiness(50.0f) {}

    // Helper methods
    bool hasCompanion(uint32_t npcId) const {
        for (const auto& comp : companionStates) {
            if (comp.companionNpcId == npcId) return true;
        }
        return false;
    }

    bool hasPet(uint32_t petId) const {
        for (const auto& pet : petStates) {
            if (pet.petNpcId == petId) return true;
        }
        return false;
    }

    bool isModActive(const std::string& modName) const {
        for (const auto& mod : activeMods) {
            if (mod == modName) return true;
        }
        return false;
    }
};

