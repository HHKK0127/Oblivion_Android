#include "player.h"
#include "fatigue_system.h"
#include <algorithm>
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
    // The base is the Skills struct's own defaults (BASE_SKILL each): one source
    // of truth, so a corrected base only has to change in player.h.
    skills = Skills{};
}

void Player::resetAttributes() {
    // The base is the Attributes struct's own defaults (BASE_ATTRIBUTE each).
    attributes = Attributes{};
}

const std::array<Player::SkillAvBinding, 21>& Player::skillAvBindings() {
    // AV index 0x0C..0x20 (Armorer..Speechcraft) -> Player::Skills member, in the
    // canonical AV order (RE_EXE_REPORT §46.1, name table 0x00B0A1A8). This is
    // the single mapping used by applyRace() and the Legacy chargen base setup.
    static const std::array<SkillAvBinding, 21> kBindings = {{
        {0x0C, &Skills::Armorer},    {0x0D, &Skills::Athletics},   {0x0E, &Skills::Blade},
        {0x0F, &Skills::Block},      {0x10, &Skills::Blunt},       {0x11, &Skills::HandToHand},
        {0x12, &Skills::HeavyArmor}, {0x13, &Skills::Alchemy},     {0x14, &Skills::Alteration},
        {0x15, &Skills::Conjuration},{0x16, &Skills::Destruction}, {0x17, &Skills::Illusion},
        {0x18, &Skills::Mysticism},  {0x19, &Skills::Restoration}, {0x1A, &Skills::Acrobatics},
        {0x1B, &Skills::LightArmor}, {0x1C, &Skills::Marksman},    {0x1D, &Skills::Mercantile},
        {0x1E, &Skills::Security},   {0x1F, &Skills::Sneak},       {0x20, &Skills::Speechcraft},
    }};
    return kBindings;
}

void Player::applyRace(const RaceBonus& rb) {
    // 1. Revert to the chargen base (no accumulation on re-apply).
    resetAttributes();
    resetSkills();
    maxHealth = BASE_MAX_HEALTH;
    health = maxHealth;
    maxMagicka = BASE_MAX_MAGICKA;
    magicka = maxMagicka;
    maxStamina = BASE_MAX_STAMINA;
    stamina = maxStamina;
    // 2. Drop the previous race's spells so a race switch leaves nothing behind.
    for (uint32_t s : race_granted_spells_) {
        auto it = std::find(knownSpells.begin(), knownSpells.end(), s);
        if (it != knownSpells.end()) knownSpells.erase(it);
    }
    race_granted_spells_.clear();
    // 3. Race skill bonuses (AV index -> field).
    for (const auto& [av, bonus] : rb.skillBonuses) {
        for (const SkillAvBinding& b : skillAvBindings()) {
            if (b.av == av) {
                skills.*(b.field) += bonus;
                break;
            }
        }
    }
    // 4. Race spells.
    for (uint32_t s : rb.spells) {
        knownSpells.push_back(s);
        race_granted_spells_.push_back(s);
    }
}
