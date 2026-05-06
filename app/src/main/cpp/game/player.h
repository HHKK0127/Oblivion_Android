#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "../world/world_data.h"
#include "item.h"

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

    // Health & Stamina
    float health = 100.0f;
    float maxHealth = 100.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;

    // Stamina Management
    float staminaDrainRate = 25.0f;      // Stamina drained per second while sprinting
    float staminaRecoveryRate = 15.0f;   // Stamina recovered per second while idle
    bool canSprint = true;               // Can only sprint if stamina > 0

    // Inventory
    uint32_t inventorySlots = 60;  // Oblivion standard
    std::vector<InventoryItem> inventory;

    // Skills & Attributes (Oblivion-style)
    uint32_t playerLevel = 1;
    float experience = 0.0f;
    float experiencePerLevelUp = 1000.0f;  // Experience needed per level

    // Skills (0-100 scale, mirroring Oblivion)
    struct Skills {
        int Blade = 10;
        int Blunt = 10;
        int Block = 10;
        int Restoration = 10;
        int Destruction = 10;
        int Alteration = 10;
        int Conjuration = 10;
        int Illusion = 10;
        int Mysticism = 10;
        int Marksman = 10;
        int Athletics = 10;
        int Acrobatics = 10;
    } skills = Skills();

    // Attributes
    struct Attributes {
        int Strength = 40;
        int Intelligence = 40;
        int Willpower = 40;
        int Agility = 40;
        int Speed = 40;
        int Endurance = 40;
        int Personality = 40;
        int Luck = 40;
    } attributes = Attributes();

    // Current Cell
    int32_t currentCellX = 0;
    int32_t currentCellY = 0;

    // Methods
    Player() = default;
    void initialize(const glm::vec3& startPos);
    void update(float deltaTime);
    void applyMovementInput(const glm::vec3& input, bool isSprinting = false);
    void applyJump();
    void updateGroundDetection(float terrainHeight);
    void takeDamage(float amount);
    void heal(float amount);
    void updateStamina(float deltaTime, bool isSprinting);  // Update stamina based on sprint state
    void reduceStamina(float amount);                        // Manually reduce stamina
    void recoverStamina(float amount);                       // Manually recover stamina

    // Level & Experience
    void addExperience(float amount);                        // Add experience
    void levelUp();                                          // Advance one level
    void setLevel(uint32_t newLevel);                        // Set level directly (for cheats)
    void maxOutAllSkills();                                  // Set all skills to 100 (for MAX_SKILLS cheat)
    void resetSkills();                                      // Reset all skills to 10
    void resetAttributes();                                  // Reset all attributes to 40
};
