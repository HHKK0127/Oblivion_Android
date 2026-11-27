#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <array>
#include <utility>
#include "../world/world_data.h"
#include "item.h"
#include "spell.h"

enum class MovementState {
    IDLE,
    WALKING,
    RUNNING,
    FALLING,
    JUMPING
};

struct Player {
    // Identity
    uint32_t playerId = 1;
    std::string name = "Player";
    // Race form reference chosen at character creation (0 = unset), carried
    // as the FormID rather than a copy of the record: the original keeps the
    // TESRace reference and applies through it (RE v365 (b)). Look the
    // record up through ESMManager::findRace when a name or bonus is needed.
    uint32_t raceFormID = 0;

    // Position & Rotation
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);  // pitch, yaw, roll

    // Velocity & Movement
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 moveInput = glm::vec3(0.0f, 0.0f, 0.0f);  // From controller
    MovementState movementState = MovementState::IDLE;

    // Physics
    float speed = 5.0f;         // Walk speed (units/sec)
    float sprintSpeed = 8.0f;   // Run speed
    float jumpForce = 10.0f;    // Jump impulse
    float gravityAccel = -9.81f; // m/s^2

    // Ground Detection
    bool isOnGround = false;
    float groundCheckRadius = 0.3f;

    // --- Chargen base (v664) ---
    // The fallback values used when no race (or a race without ATTR) supplies
    // its own. A race's attributes ARE its base (RACE ATTR, V-664-2), so
    // applyRace() overwrites these; BASE_ATTRIBUTE only survives for a race
    // that carries no ATTR. These are the ONE definition: the member
    // initializers below use them, resetAttributes()/resetSkills() and
    // applyRace() revert to them, and the Legacy chargen reads them, so the
    // Panel and Legacy paths cannot drift apart.
    static constexpr int BASE_ATTRIBUTE = 40;
    static constexpr int BASE_SKILL = 10;
    static constexpr float BASE_MAX_HEALTH = 100.0f;
    static constexpr float BASE_MAX_MAGICKA = 100.0f;
    static constexpr float BASE_MAX_STAMINA = 100.0f;

    // Health, Magicka & Stamina
    float health = BASE_MAX_HEALTH;
    float maxHealth = BASE_MAX_HEALTH;
    float magicka = BASE_MAX_MAGICKA;
    float maxMagicka = BASE_MAX_MAGICKA;
    float stamina = BASE_MAX_STAMINA;
    float maxStamina = BASE_MAX_STAMINA;

    // Identity flags
    uint8_t gender = 0;  // 0=Male, 1=Female
    // One place for the 0/1 test: the Gender toggle flips this and applyRace's
    // mapping picks the male/female ATTR row from it, so the comparison is not
    // scattered as `== 1` (the inversion was already hit once, V-631-1b).
    bool isFemale() const { return gender != 0; }

    // Stamina Management
    // Fatigue drain/recovery rates are driven by the GMST formulas in
    // game/fatigue_system.h (RE §50.2), not by fixed constants here.
    bool canSprint = true;               // Can only sprint if stamina > 0

    // Inventory lives in the global Inventory owned by InventoryManager; see
    // game/inventory.h. Player no longer keeps a second copy, so every screen
    // that shows or spends carried items reads the same store.

    // Skills & Attributes (Oblivion-style)
    uint32_t playerLevel = 1;
    float experience = 0.0f;
    float experiencePerLevelUp = 1000.0f;  // Experience needed per level

    // Skills (0-100 scale, mirroring Oblivion)
    struct Skills {
        int Armorer = BASE_SKILL;
        int Athletics = BASE_SKILL;
        int Blade = BASE_SKILL;
        int Block = BASE_SKILL;
        int Blunt = BASE_SKILL;
        int HandToHand = BASE_SKILL;
        int HeavyArmor = BASE_SKILL;
        int Alchemy = BASE_SKILL;
        int Alteration = BASE_SKILL;
        int Conjuration = BASE_SKILL;
        int Destruction = BASE_SKILL;
        int Illusion = BASE_SKILL;
        int Mysticism = BASE_SKILL;
        int Restoration = BASE_SKILL;
        int Acrobatics = BASE_SKILL;
        int LightArmor = BASE_SKILL;
        int Marksman = BASE_SKILL;
        int Mercantile = BASE_SKILL;
        int Security = BASE_SKILL;
        int Sneak = BASE_SKILL;
        int Speechcraft = BASE_SKILL;
    } skills = Skills();

    // Attributes
    struct Attributes {
        int Strength = BASE_ATTRIBUTE;
        int Intelligence = BASE_ATTRIBUTE;
        int Willpower = BASE_ATTRIBUTE;
        int Agility = BASE_ATTRIBUTE;
        int Speed = BASE_ATTRIBUTE;
        int Endurance = BASE_ATTRIBUTE;
        int Personality = BASE_ATTRIBUTE;
        int Luck = BASE_ATTRIBUTE;
    } attributes = Attributes();

    // Current Cell
    int32_t currentCellX = 0;
    int32_t currentCellY = 0;

    // Known spells (player's spell book)
    std::vector<uint32_t> knownSpells;
    std::vector<uint32_t> equippedSpells;

    // Quick-slot spells (4 slots, nullptr = empty)
    static constexpr int QUICK_SLOT_COUNT = 4;
    std::array<std::shared_ptr<Spell>, QUICK_SLOT_COUNT> quickSlotSpells = {};

    // Methods
    Player() = default;
    void initialize(const glm::vec3& startPos);
    void update(float deltaTime);
    void applyMovementInput(const glm::vec3& input, bool isSprinting = false);
    void applyJump();
    void updateGroundDetection(float terrainHeight);
    void takeDamage(float amount);
    void heal(float amount);

    // Gameplay difficulty scales incoming damage (GameplayMenu difficulty slider).
    // 1.0 is neutral; see difficultyDamageMultiplier(). RE §50.1.
    static void setDifficultyScale(float scale) {
        difficultyScale = (scale > 0.0f) ? scale : 1.0f;
    }
    static float getDifficultyScale() { return difficultyScale; }
    // Damage the player deals is scaled by the reciprocal of the incoming scale:
    // FUN_005E2560 divides by the difficulty multiplier when the player attacks
    // (RE §50.1).
    static float getDealtDamageScale() { return 1.0f / difficultyScale; }
    void updateStamina(float deltaTime, bool isSprinting);  // Update stamina based on sprint state
    void reduceStamina(float amount);                        // Manually reduce stamina
    void recoverStamina(float amount);                       // Manually recover stamina

    // Level & Experience
    void addExperience(float amount);                        // Add experience
    void levelUp();                                          // Advance one level
    void setLevel(uint32_t newLevel);                        // Set level directly (for cheats)
    void maxOutAllSkills();                                  // Set all skills to 100 (for MAX_SKILLS cheat)
    void resetSkills();                                      // Reset all skills to the base
    void resetAttributes();                                  // Reset all attributes to the base

    // --- Race application (v664) ---
    // Skill AV index (0x0C..0x20) -> Player::Skills member, in AV order. ONE
    // mapping, shared by applyRace() and the Legacy chargen base setup.
    struct SkillAvBinding {
        uint8_t av;
        int Skills::* field;
    };
    static const std::array<SkillAvBinding, 21>& skillAvBindings();

    // The race-derived inputs applyRace() needs, kept free of the ESM reader so
    // the pure logic stays host-testable; the owner (NpcManager) maps RaceData
    // onto it. Health is not filled yet (V-664-3).
    struct RaceBonus {
        std::vector<std::pair<uint8_t, int8_t>> skillBonuses;  // AV 0x0C..0x20, bonus
        std::vector<uint32_t> spells;                          // SPLO formIDs
        // Base attributes for the player's sex, in RaceData::AttrIndex order
        // (Strength..Luck). The owner picks the male/female row once, at the
        // mapping site, so applyRace() never sees the sex. hasAttr=false (a
        // race without ATTR) leaves the BASE_ATTRIBUTE fallback in place.
        bool hasAttr = false;
        std::array<uint8_t, 8> attrs{};
    };
    // Revert to the chargen base, then apply the race. Idempotent: re-applying
    // the same race does not double a bonus, and a different race leaves nothing
    // behind (the previous race's spells are removed first).
    void applyRace(const RaceBonus& rb);
    // Spell FormIDs granted by the last applyRace() (tests / callers).
    const std::vector<uint32_t>& raceGrantedSpells() const { return race_granted_spells_; }

private:
    static inline float difficultyScale = 1.0f;
    // Spells granted by the last applyRace(); removed on the next apply.
    std::vector<uint32_t> race_granted_spells_;
};
