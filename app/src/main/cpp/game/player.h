#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "../world/world_data.h"

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

    // Inventory
    uint32_t inventorySlots = 60;  // Oblivion standard
    std::vector<struct InventoryItem> inventory;

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
};
