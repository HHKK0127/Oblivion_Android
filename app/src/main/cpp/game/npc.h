#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

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

    // Methods
    CharacterStatus();
    void initialize(float baseHealth, float baseMana, uint32_t level);
    bool isAlive() const { return currentHealth > 0.0f; }
    void takeDamage(float amount);
    void heal(float amount);
    float getAttributeBonus(const std::string& attr) const;
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

    // Combat System
    CharacterStatus status;
    float lastDamageTime;
    bool inCombat;
    std::shared_ptr<NPC> combatTarget;  // Current combat opponent
    float combatEngagementTime;

    // Quest System
    std::vector<uint32_t> availableQuests;  // Quest IDs this NPC can give
    std::vector<uint32_t> givenQuests;      // Quests already given

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
    void moveTo(const glm::vec3& target);

    void addQuestToOffer(uint32_t questId);
    std::vector<uint32_t> getOfferedQuests() const;
    bool hasCompletedQuest(uint32_t questId) const;

    // Magic System
    float lastSpellCastTime;
    uint32_t selectSpellForCombat();  // 戦闘時のスペル選択AI
    bool canCastSpell(uint32_t spellId) const;

    // Graphics System
    void updateModelMatrix();  // Update modelMatrix from position/rotation
};
