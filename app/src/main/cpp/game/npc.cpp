#include "npc.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "NPC"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// CharacterStatus Implementation
CharacterStatus::CharacterStatus()
    : currentHealth(100.0f), maxHealth(100.0f),
      currentMana(50.0f), maxMana(50.0f),
      stamina(100.0f), maxStamina(100.0f),
      equippedWeaponId(0), weaponDamage(10.0f), armorRating(5.0f) {
}

void CharacterStatus::initialize(float baseHealth, float baseMana, uint32_t level) {
    maxHealth = baseHealth * (1.0f + level * 0.1f);
    currentHealth = maxHealth;

    maxMana = baseMana * (1.0f + level * 0.1f);
    currentMana = maxMana;

    maxStamina = 100.0f;
    stamina = maxStamina;

    // Initialize default attributes
    attributes["Strength"] = 50.0f + level * 2;
    attributes["Intelligence"] = 50.0f + level * 2;
    attributes["Willpower"] = 50.0f + level * 2;
    attributes["Agility"] = 50.0f + level * 2;
    attributes["Speed"] = 50.0f + level * 2;
    attributes["Endurance"] = 50.0f + level * 2;
    attributes["Personality"] = 50.0f + level * 2;
    attributes["Luck"] = 50.0f + level * 2;

    // Initialize default skills
    skills["Blade"] = 30.0f + level * 3;
    skills["Blunt"] = 30.0f + level * 3;
    skills["Magic"] = 30.0f + level * 3;
    skills["Restoration"] = 30.0f + level * 3;
}

void CharacterStatus::takeDamage(float amount) {
    if (amount < 0) return;
    currentHealth -= amount;
    if (currentHealth < 0) currentHealth = 0;
}

void CharacterStatus::heal(float amount) {
    if (amount < 0) return;
    currentHealth += amount;
    if (currentHealth > maxHealth) currentHealth = maxHealth;
}

float CharacterStatus::getAttributeBonus(const std::string& attr) const {
    auto it = attributes.find(attr);
    if (it != attributes.end()) {
        return (it->second - 50.0f) / 10.0f;  // Bonus per 10 points
    }
    return 0.0f;
}

// NPC Implementation
NPC::NPC(uint32_t id, const std::string& n)
    : npcId(id), name(n), race("Human"), class_("Warrior"),
      position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f),
      moveSpeed(5.0f),
      aiState(AIState::IDLE),
      targetPosition(0.0f, 0.0f, 0.0f),
      wanderRadius(10.0f),
      lastDamageTime(0.0f),
      inCombat(false),
      combatEngagementTime(0.0f) {

    // Initialize status
    status.initialize(100.0f, 50.0f, 1);
    lastSpellCastTime = 0.0f;
    LOGD("NPC created: ID=%u, Name=%s", npcId, name.c_str());
}

NPC::~NPC() {
    LOGD("NPC destroyed: ID=%u, Name=%s", npcId, name.c_str());
}

void NPC::update(float deltaTime) {
    switch (aiState) {
        case AIState::IDLE:
            // Just stand there
            break;

        case AIState::WANDER:
            // TODO: Implement random wandering
            break;

        case AIState::PATROL:
            // TODO: Implement patrol waypoint following
            break;

        case AIState::FOLLOW_PLAYER:
            // TODO: Implement player following
            break;

        case AIState::COMBAT:
            if (combatTarget && !combatTarget->status.isAlive()) {
                exitCombat();
            }
            break;

        default:
            break;
    }

    // Regenerate mana slowly
    if (status.currentMana < status.maxMana) {
        status.currentMana += status.maxMana * 0.1f * deltaTime;
        if (status.currentMana > status.maxMana) {
            status.currentMana = status.maxMana;
        }
    }

    // Update model matrix for graphics
    updateModelMatrix();
}

void NPC::takeDamage(float amount) {
    status.takeDamage(amount);
    lastDamageTime = 0.0f;  // Reset damage timer

    if (!status.isAlive()) {
        LOGI("NPC defeated: ID=%u, Name=%s", npcId, name.c_str());
        aiState = AIState::IDLE;
        if (inCombat) {
            exitCombat();
        }
    }
}

void NPC::heal(float amount) {
    status.heal(amount);
}

float NPC::getAttackPower() const {
    float strengthBonus = status.getAttributeBonus("Strength");
    return status.weaponDamage + strengthBonus * 2.0f;
}

bool NPC::canAttack() const {
    return status.isAlive() && inCombat && combatTarget;
}

void NPC::enterCombat(std::shared_ptr<NPC> opponent) {
    inCombat = true;
    combatTarget = opponent;
    aiState = AIState::COMBAT;
    combatEngagementTime = 0.0f;
    LOGI("NPC entered combat: ID=%u, Target=%u", npcId,
         opponent ? opponent->npcId : 0);
}

void NPC::exitCombat() {
    inCombat = false;
    combatTarget = nullptr;
    aiState = AIState::IDLE;
    LOGI("NPC exited combat: ID=%u", npcId);
}

void NPC::setAIState(AIState newState) {
    if (aiState != newState) {
        aiState = newState;
        LOGD("NPC AI state changed: ID=%u, State=%d", npcId, static_cast<int>(newState));
    }
}

void NPC::moveTo(const glm::vec3& target) {
    targetPosition = target;
    aiState = AIState::FOLLOW_PLAYER;  // Use this for movement

    glm::vec3 direction = target - position;
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

    if (distance > 0.1f) {
        if (distance > 0.0f) {
            direction = direction * (1.0f / distance);  // Manual normalize
        }
        position = position + direction * moveSpeed * 0.016f;  // Assume 60fps
    }
}

void NPC::addQuestToOffer(uint32_t questId) {
    availableQuests.push_back(questId);
}

std::vector<uint32_t> NPC::getOfferedQuests() const {
    return availableQuests;
}

bool NPC::hasCompletedQuest(uint32_t questId) const {
    auto it = std::find(givenQuests.begin(), givenQuests.end(), questId);
    return it != givenQuests.end();
}

uint32_t NPC::selectSpellForCombat() {
    // スペルキャスト間隔（1.5秒）をチェック
    if (lastSpellCastTime < 1.5f) {
        return 0;  // スペルキャスト不可
    }

    // 装備されたスペルから選択
    if (status.equippedSpells.empty()) {
        return 0;  // 装備スペルなし
    }

    // HP が低い時 (< 30%) → ヒール系を優先
    if (status.currentHealth < status.maxHealth * 0.3f) {
        for (uint32_t spellId : status.equippedSpells) {
            // スペルIDが2001の場合（ヒール）
            if (spellId == 2001 && status.currentMana >= 40.0f) {
                LOGD("NPC %u selecting Heal (low HP: %.1f/%.1f)",
                     npcId, status.currentHealth, status.maxHealth);
                return 2001;  // ヒール
            }
        }
    }

    // マナが低い時 (< 30%) → マナ回復優先
    if (status.currentMana < status.maxMana * 0.3f) {
        for (uint32_t spellId : status.equippedSpells) {
            // スペルIDが2002の場合（マナ回復）
            if (spellId == 2002 && status.currentMana >= 30.0f) {
                LOGD("NPC %u selecting Restore Mana (low mana: %.1f/%.1f)",
                     npcId, status.currentMana, status.maxMana);
                return 2002;  // マナ回復
            }
        }
    }

    // デフォルト: ダメージスペルを選択（マナが余っている場合）
    for (uint32_t spellId : status.equippedSpells) {
        // スペルIDが2000の場合（ファイアボール）
        if (spellId == 2000 && status.currentMana >= 50.0f) {
            LOGD("NPC %u selecting Fireball (offensive)", npcId);
            return 2000;  // ファイアボール
        }
    }

    return 0;  // キャストできるスペルなし
}

bool NPC::canCastSpell(uint32_t spellId) const {
    // スペルIDが装備スペルに含まれているか確認
    auto it = std::find(status.equippedSpells.begin(),
                       status.equippedSpells.end(), spellId);
    return it != status.equippedSpells.end();
}

void NPC::updateModelMatrix() {
    // Create identity matrix by translating at origin
    modelMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));

    // Translate to position
    modelMatrix = glm::translate(modelMatrix, position);

    // Apply rotations (X-Z-Y euler angle convention: pitch, yaw, roll)
    // Pitch (X-axis rotation)
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

    // Yaw (Y-axis rotation)
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

    // Roll (Z-axis rotation)
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
}
