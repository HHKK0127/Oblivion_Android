#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

// Forward declarations
class Mesh;

enum class AIState {
    // Phase 1 - Basic Movement AI
    IDLE,           // Standing/waiting
    WANDER,         // Random movement
    PATROL,         // Patrolling along waypoints
    FOLLOW_PLAYER,  // Following player
    COMBAT,         // In combat with enemy

    // Phase 3 - Advanced States
    CONFUSED,       // 混乱状態（攻撃対象が定まらない）
    PANICKED,       // パニック状態（恐怖で動けない）
    FLEEING,        // 逃走状態（危機的HP）
    CONVERSATION,   // 会話中
    WAITING,        // 待機状態
    QUEST_OBJECTIVE // クエスト目標達成中
};

struct CharacterStatus {
    // Core Vitals
    float currentHealth;
    float maxHealth;
    float currentMana;
    float maxMana;
    float stamina;
    float maxStamina;

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

    // Rendering
    std::shared_ptr<Mesh> mesh;
    std::string meshAssetPath;
    void updateModelMatrix();
    glm::mat4 modelMatrix;

    // Magic System
    float lastSpellCastTime;
    uint32_t selectSpellForCombat();  // 戦闘時のスペル選択AI
    bool canCastSpell(uint32_t spellId) const;

    // Phase 2: Advanced Combat AI - Spell Selection
    enum class SpellType {
        DAMAGE,         // ダメージスペル（ファイアボール等）
        RESTORATION,    // 回復スペル
        MANA_RESTORE,   // マナ回復スペル
        FORTIFY,        // 強化スペル
        DEBUFF,         // 弱体化スペル
        UNKNOWN         // 不明
    };

    uint32_t findSpellByType(SpellType type);
    uint32_t findMostEffectiveSpell(std::shared_ptr<NPC> enemy);
    bool hasBuffActive(const std::string& buffName) const;
    bool shouldFlee() const;

    // Phase 4: Dialogue System
    bool inConversation = false;
    std::string currentDialogueNodeId;

    bool startConversation();
    void endConversation();
    bool isInConversation() const { return inConversation; }

    // Phase 1: Movement AI (WANDER/PATROL/FOLLOW)
    // PATROL: Waypoint patrol system
    std::vector<glm::vec3> patrolWaypoints;
    int currentWaypointIndex = 0;
    float waypointArrivalThreshold = 1.0f;

    // WANDER: Random movement system
    float lastWanderTargetTime = 0.0f;
    static constexpr float WANDER_TARGET_UPDATE_TIME = 5.0f;
    static constexpr float ENEMY_DETECTION_RADIUS = 30.0f;
    static constexpr float FOLLOW_PLAYER_DESIRED_DISTANCE = 3.0f;

    // Movement AI Helper Methods
    void generateRandomWanderTarget();
    void moveTowardsTarget(float deltaTime);
    bool shouldDetectEnemies();
};
