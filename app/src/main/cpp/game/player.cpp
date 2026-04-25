#include "player.h"
#include <cmath>

void Player::initialize(const glm::vec3& startPos) {
    position = startPos;
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    health = maxHealth;
    stamina = maxStamina;
}

void Player::update(float deltaTime) {
    // Update movement state
    float speed_sq = velocity.x * velocity.x + velocity.z * velocity.z;
    if (speed_sq > 0.01f) {
        movementState = MovementState::WALKING;
    } else {
        movementState = MovementState::IDLE;
    }

    if (velocity.y < -0.1f) {
        movementState = MovementState::FALLING;
    }
}

void Player::applyMovementInput(const glm::vec3& input, bool isSprinting) {
    // Calculate horizontal velocity
    float currentSpeed = isSprinting ? sprintSpeed : speed;

    // Apply rotation to movement (forward/backward relative to player facing)
    float yaw = rotation.y * 3.14159f / 180.0f;

    float cosYaw = std::cos(yaw);
    float sinYaw = std::sin(yaw);

    // Forward/back from input.z
    velocity.x = (input.z * cosYaw - input.x * sinYaw) * currentSpeed;
    velocity.z = (input.z * sinYaw + input.x * cosYaw) * currentSpeed;
}

void Player::applyJump() {
    if (isOnGround) {
        velocity.y = jumpForce;
        isOnGround = false;
    }
}

void Player::updateGroundDetection(float terrainHeight) {
    isOnGround = (position.y <= terrainHeight + groundCheckRadius);
}

void Player::takeDamage(float amount) {
    health -= amount;
    if (health < 0.0f) health = 0.0f;
}

void Player::heal(float amount) {
    health += amount;
    if (health > maxHealth) health = maxHealth;
}
