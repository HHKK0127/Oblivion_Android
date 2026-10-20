#include "player.h"
#include "fatigue_system.h"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "Player"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

void Player::initialize(const glm::vec3& startPos) {
    position = startPos;
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    health = maxHealth;
    stamina = maxStamina;
}

void Player::update(float) {
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
        // Jump fatigue cost (0x671620): 4 + 4x, x = current fatigue ratio.
        // Computed before the drain so the ratio reflects the pre-jump value.
        const float x = oblivion::fatigue::ratio(stamina, maxStamina);
        reduceStamina(oblivion::fatigue::jumpCost(x));
        velocity.y = jumpForce;
        isOnGround = false;
    }
}

void Player::updateGroundDetection(float terrainHeight) {
    isOnGround = (position.y <= terrainHeight + groundCheckRadius);
}

void Player::takeDamage(float amount) {
    // RE_EXE_REPORT 41.2.4: the HP write clamps anything below 1.0 to
    // exactly 0.0 and treats that landing as death. difficultyScale is the
    // received-damage factor f (41.4). The original runs the actor death
    // pipeline at that point; the port has no player death screen yet, so
    // this only records the transition.
    float previous = health;
    health -= amount * difficultyScale;
    if (health < 1.0f) health = 0.0f;
    if (previous >= 1.0f && health == 0.0f) {
        LOGW("Player died: HP reached 0 (incoming %.1f)", amount);
    }
}

void Player::heal(float amount) {
    health += amount;
    if (health > maxHealth) health = maxHealth;
}

void Player::updateStamina(float deltaTime, bool isSprinting) {
    // Fatigue AV (RE §50.2). Sprinting pays the 0x5FAAE0 athletics-scaled run
    // drain; idle recovers the flat 0x5F2720 return rate. Athletics 80+ makes
    // the run cost zero, so sprinting no longer drains a high-skill player.
    if (isSprinting) {
        const float athletics = static_cast<float>(skills.Athletics);
        stamina -= oblivion::fatigue::runCostPerSecond(athletics) * deltaTime;
        if (stamina < 0.0f) stamina = 0.0f;
        canSprint = (stamina > 0.0f);
    } else {
        const float endurance = static_cast<float>(attributes.Endurance);
        stamina += oblivion::fatigue::recoverPerSecond(endurance) * deltaTime;
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
    // All 21 canonical skills (RE_EXE_REPORT §46.1); a skill left at its default
    // here would make the corresponding dialogue actor-value condition read stale.
    skills.Armorer = 100;
    skills.Athletics = 100;
    skills.Blade = 100;
    skills.Block = 100;
    skills.Blunt = 100;
    skills.HandToHand = 100;
    skills.HeavyArmor = 100;
    skills.Alchemy = 100;
    skills.Alteration = 100;
    skills.Conjuration = 100;
    skills.Destruction = 100;
    skills.Illusion = 100;
    skills.Mysticism = 100;
    skills.Restoration = 100;
    skills.Acrobatics = 100;
    skills.LightArmor = 100;
    skills.Marksman = 100;
    skills.Mercantile = 100;
    skills.Security = 100;
    skills.Sneak = 100;
    skills.Speechcraft = 100;
}

void Player::resetSkills() {
    skills.Armorer = 10;
    skills.Athletics = 10;
    skills.Blade = 10;
    skills.Block = 10;
    skills.Blunt = 10;
    skills.HandToHand = 10;
    skills.HeavyArmor = 10;
    skills.Alchemy = 10;
    skills.Alteration = 10;
    skills.Conjuration = 10;
    skills.Destruction = 10;
    skills.Illusion = 10;
    skills.Mysticism = 10;
    skills.Restoration = 10;
    skills.Acrobatics = 10;
    skills.LightArmor = 10;
    skills.Marksman = 10;
    skills.Mercantile = 10;
    skills.Security = 10;
    skills.Sneak = 10;
    skills.Speechcraft = 10;
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
