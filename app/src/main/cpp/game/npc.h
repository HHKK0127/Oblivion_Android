#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace oblivion {
    class NavMeshManager;  // Forward declaration
}

enum class AIState {
    IDLE,           // Standing/waiting
    WANDER,         // Random movement
    PATROL,         // Patrolling along waypoints
    FOLLOW_PLAYER,  // Following player
    COMBAT          // In combat with enemy
};

struct CharacterStatus {
    // Core Vitals
    float currentHealth;
    float maxHealth;
    float currentMana;
    float maxMana;
    float stamina;
    float maxStamina;

    // Level & Experience
    uint32_t level = 1;
    float experience = 0.0f;

    // Attributes (Oblivion 8 attributes)
    std::unordered_map<std::string, float> attributes;  // Strength, Intelligence, etc.

    // Skills
    std::unordered_map<std::string, float> skills;  // Blade, Magic, etc.

    // Equipment & Combat
    uint32_t equippedWeaponId;
    float weaponDamage;
    float armorRating;

    // Magic System
    std::vector<uint32_t> knownSpells;     // Spell IDs this NPC can cast
    std::vector<uint32_t> equippedSpells;  // Currently equipped spells

    // Oblivion Actor Value (AV) IDs. The first three map to the derived
        // vitals; 0-7 map to the attributes. See dialogue_filter_engine.cpp for
        // the full skill/attribute AV id table.
        static constexpr uint32_t AV_STRENGTH     = 0;
        static constexpr uint32_t AV_INTELLIGENCE = 1;
        static constexpr uint32_t AV_WILLPOWER    = 2;
        static constexpr uint32_t AV_AGILITY      = 3;
        static constexpr uint32_t AV_SPEED        = 4;
        static constexpr uint32_t AV_ENDURANCE    = 5;
        static constexpr uint32_t AV_PERSONALITY  = 6;
        static constexpr uint32_t AV_LUCK         = 7;
        static constexpr uint32_t AV_HEALTH       = 8;
        static constexpr uint32_t AV_MAGICKA      = 9;
        static constexpr uint32_t AV_STAMINA      = 10;

        // Methods
        CharacterStatus();
        void initialize(float baseHealth, float baseMana, uint32_t level);
        bool isAlive() const { return currentHealth > 0.0f; }
        void takeDamage(float amount);
        void heal(float amount);
        float getAttributeBonus(const std::string& attr) const;

        // Real AV read accessor. Mirrors the confirmed RE behavior (RE_EXE_REPORT
        // §17.11.7): AV values are stored in an equipped magic-handle object and
        // read through GetActorValue. AV 8 (Health) applies the current/max ratio
        // ("magicka multiplier"); a zero max is guarded to a ratio of 1.0.
        float getActorValue(uint32_t avId) const;
    };

struct NPC {
    uint32_t npcId;
    std::string name;
    std::string race;
    std::string class_;

    // Position & Movement
    glm::vec3 position;
    glm::vec3 rotation;
    float moveSpeed;

    // Graphics System (Mesh Binding)
    std::shared_ptr<class Mesh> mesh;           // 3D mesh for rendering
    std::string meshAssetPath;                  // Asset path: e.g., "meshes/creatures/imp.nif"
    glm::mat4 modelMatrix;                      // Model matrix (computed from position/rotation)

    // AI & Behavior
    AIState aiState;
    glm::vec3 targetPosition;
    float wanderRadius;

    // NavMesh Pathfinding
    std::vector<glm::vec3> currentPath;  // Current path to follow
    int currentPathIndex;                 // Current waypoint index
    float pathReachThreshold;             // Distance to consider waypoint reached

    // Combat System
    CharacterStatus status;
    float lastDamageTime;
    bool inCombat;
    std::shared_ptr<NPC> combatTarget;  // Current combat opponent
    float combatEngagementTime;

    // Animation State (for Imperial Weave integration)
    enum class AnimState : uint8_t {
        IDLE,
        WALK,
        RUN,
        ATTACK,
        HIT_REACTION,
        BLOCK,
        DEATH
    };
    AnimState animState = AnimState::IDLE;
    float animTimer = 0.0f;
    static constexpr float HIT_REACTION_DURATION = 0.5f;
    static constexpr float ATTACK_DURATION = 0.8f;

    // FaceGen Properties (Phase 52)
    bool isFemale = false;
    float age = 30.0f;
    std::string texturePath;  // Face texture override

    // Quest System
    std::vector<uint32_t> availableQuests;  // Quest IDs this NPC can give
    std::vector<uint32_t> givenQuests;      // Quests already given

    // ESM linkage
    uint32_t formID = 0;                    // Base record FormID (NPC_/CREA), 0 if procedural
    std::vector<uint32_t> factionFormIDs;   // Faction memberships (for dialogue branching)

    // Methods
    NPC(uint32_t id, const std::string& n);
    ~NPC();

    void update(float deltaTime);
    void takeDamage(float amount);
    void heal(float amount);
    float getAttackPower() const;
    bool canAttack() const;
    void enterCombat(std::shared_ptr<NPC> opponent);
    void exitCombat();
    void setAIState(AIState newState);
    void moveTo(const glm::vec3& target, const oblivion::NavMeshManager* navMesh = nullptr);

    void addQuestToOffer(uint32_t questId);
    std::vector<uint32_t> getOfferedQuests() const;
    bool hasCompletedQuest(uint32_t questId) const;

    // Animation State Management
    void triggerHitReaction();
    void triggerAttack();
    void triggerBlock();
    void triggerDeath();
    void updateAnimState(float deltaTime);
    AnimState getAnimState() const { return animState; }

    // Magic System
    float lastSpellCastTime;
    uint32_t selectSpellForCombat();  // Spell selection AI for combat
    bool canCastSpell(uint32_t spellId) const;

    // Graphics System
    void updateModelMatrix();  // Update modelMatrix from position/rotation
};
