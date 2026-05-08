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

void Player::updateStamina(float deltaTime, bool isSprinting) {
    if (isSprinting && stamina > 0.0f) {
        // Drain stamina while sprinting
        stamina -= staminaDrainRate * deltaTime;
        if (stamina < 0.0f) stamina = 0.0f;
        canSprint = (stamina > 0.0f);
    } else {
        // Recover stamina while not sprinting
        stamina += staminaRecoveryRate * deltaTime;
        if (stamina > maxStamina) stamina = maxStamina;
        canSprint = true;
    }
}

void Player::reduceStamina(float amount) {
    stamina -= amount;
    if (stamina < 0.0f) stamina = 0.0f;
    canSprint = (stamina > 0.0f);
}

void Player::recoverStamina(float amount) {
    stamina += amount;
    if (stamina > maxStamina) stamina = maxStamina;
}

void Player::addExperience(float amount) {
    experience += amount;
    if (experience >= experiencePerLevelUp) {
        levelUp();
    }
}

void Player::levelUp() {
    playerLevel++;
    experience = 0.0f;
    // Increase experience requirement for next level
    experiencePerLevelUp *= 1.1f;  // 10% increase per level
}

void Player::setLevel(uint32_t newLevel) {
    playerLevel = newLevel;
    experience = 0.0f;
    // Recalculate experience requirement based on level
    experiencePerLevelUp = 1000.0f;
    for (uint32_t i = 1; i < newLevel; i++) {
        experiencePerLevelUp *= 1.1f;
    }
}

void Player::maxOutAllSkills() {
    skills.Blade = 100;
    skills.Blunt = 100;
    skills.Block = 100;
    skills.Restoration = 100;
    skills.Destruction = 100;
    skills.Alteration = 100;
    skills.Conjuration = 100;
    skills.Illusion = 100;
    skills.Mysticism = 100;
    skills.Marksman = 100;
    skills.Athletics = 100;
    skills.Acrobatics = 100;
}

void Player::resetSkills() {
    skills.Blade = 10;
    skills.Blunt = 10;
    skills.Block = 10;
    skills.Restoration = 10;
    skills.Destruction = 10;
    skills.Alteration = 10;
    skills.Conjuration = 10;
    skills.Illusion = 10;
    skills.Mysticism = 10;
    skills.Marksman = 10;
    skills.Athletics = 10;
    skills.Acrobatics = 10;
}

void Player::resetAttributes() {
    attributes.Strength = 40;
    attributes.Intelligence = 40;
    attributes.Willpower = 40;
    attributes.Agility = 40;
    attributes.Speed = 40;
    attributes.Endurance = 40;
    attributes.Personality = 40;
    attributes.Luck = 40;
}
