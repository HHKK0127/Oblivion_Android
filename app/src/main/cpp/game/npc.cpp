#include "npc.h"
#include "navmesh_manager.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "NPC"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
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
      currentPath(),
      currentPathIndex(0),
      pathReachThreshold(0.5f),
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
            // Follow NavMesh path if available
            if (!currentPath.empty() && currentPathIndex < static_cast<int>(currentPath.size())) {
                glm::vec3 waypoint = currentPath[currentPathIndex];
                glm::vec3 direction = waypoint - position;
                float distanceSq = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;

                if (distanceSq < pathReachThreshold * pathReachThreshold) {
                    // Reached waypoint, move to next
                    currentPathIndex++;
                    if (currentPathIndex >= static_cast<int>(currentPath.size())) {
                        // Reached end of path
                        currentPath.clear();
                        currentPathIndex = 0;
                        aiState = AIState::IDLE;
                    }
                } else {
                    // Move toward waypoint
                    float distance = std::sqrt(distanceSq);
                    if (distance > 0.0f) {
                        direction.x /= distance;
                        direction.y /= distance;
                        direction.z /= distance;
                    }
                    position.x += direction.x * moveSpeed * deltaTime;
                    position.y += direction.y * moveSpeed * deltaTime;
                    position.z += direction.z * moveSpeed * deltaTime;

                    // Update rotation to face movement direction
                    rotation.y = std::atan2(direction.x, direction.z) * (180.0f / 3.14159265f);
                }
            } else {
                // No path, move directly toward target
                glm::vec3 direction = targetPosition - position;
                float distanceSq = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;

                if (distanceSq > 0.01f) {
                    float distance = std::sqrt(distanceSq);
                    if (distance > 0.0f) {
                        direction.x /= distance;
                        direction.y /= distance;
                        direction.z /= distance;
                    }
                    position.x += direction.x * moveSpeed * deltaTime;
                    position.y += direction.y * moveSpeed * deltaTime;
                    position.z += direction.z * moveSpeed * deltaTime;
                } else {
                    aiState = AIState::IDLE;
                }
            }
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

    // Update animation state
    updateAnimState(deltaTime);

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

// ============================================================================
// Animation State Management
// ============================================================================
void NPC::triggerHitReaction() {
    if (animState == AnimState::DEATH) return;  // Can't react if dead
    animState = AnimState::HIT_REACTION;
    animTimer = HIT_REACTION_DURATION;
    LOGD("NPC %s hit reaction triggered", name.c_str());
}

void NPC::triggerAttack() {
    if (animState == AnimState::DEATH) return;
    animState = AnimState::ATTACK;
    animTimer = ATTACK_DURATION;
    LOGD("NPC %s attack animation triggered", name.c_str());
}

void NPC::triggerBlock() {
    if (animState == AnimState::DEATH) return;
    animState = AnimState::BLOCK;
    LOGD("NPC %s block animation triggered", name.c_str());
}

void NPC::triggerDeath() {
    animState = AnimState::DEATH;
    animTimer = 0.0f;
    LOGD("NPC %s death animation triggered", name.c_str());
}

void NPC::updateAnimState(float deltaTime) {
    if (animTimer > 0.0f) {
        animTimer -= deltaTime;
        if (animTimer <= 0.0f) {
            animTimer = 0.0f;
            // Return to idle after animation completes
            if (animState == AnimState::HIT_REACTION || animState == AnimState::ATTACK) {
                animState = AnimState::IDLE;
            }
        }
    }
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

void NPC::moveTo(const glm::vec3& target, const oblivion::NavMeshManager* navMesh) {
    targetPosition = target;
    aiState = AIState::FOLLOW_PLAYER;

    // Use NavMesh pathfinding if available
    if (navMesh) {
        std::vector<glm::vec3> path;
        if (navMesh->findPath(position, target, path)) {
            currentPath = path;
            currentPathIndex = 0;
            LOGD("NPC %s: Path found with %zu waypoints", name.c_str(), currentPath.size());
            return;
        }
        LOGD("NPC %s: No NavMesh path found, using direct movement", name.c_str());
    }

    // Fallback: direct movement (no pathfinding)
    currentPath.clear();
    currentPathIndex = 0;
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
