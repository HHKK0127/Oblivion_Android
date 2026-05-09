#include "npc.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

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
            {
                lastWanderTargetTime += deltaTime;
                if (lastWanderTargetTime >= WANDER_TARGET_UPDATE_TIME) {
                    generateRandomWanderTarget();
                    lastWanderTargetTime = 0.0f;
                }

                // Check for enemies while wandering
                if (shouldDetectEnemies()) {
                    // Enemy detection will be handled by combat system
                }

                moveTowardsTarget(deltaTime);
            }
            break;

        case AIState::PATROL:
            {
                if (!patrolWaypoints.empty()) {
                    glm::vec3 currentWaypoint = patrolWaypoints[currentWaypointIndex];
                    float distToWaypoint = glm::distance(position, currentWaypoint);

                    // Check if arrived at waypoint
                    if (distToWaypoint <= waypointArrivalThreshold) {
                        currentWaypointIndex = (currentWaypointIndex + 1) % patrolWaypoints.size();
                        LOGD("NPC %u reached waypoint %d, moving to next", npcId, currentWaypointIndex - 1);
                    }

                    // Move toward current waypoint
                    targetPosition = patrolWaypoints[currentWaypointIndex];
                    moveTowardsTarget(deltaTime);

                    // Check for enemies while patrolling
                    if (shouldDetectEnemies()) {
                        // Enemy detection will be handled by combat system
                    }
                } else {
                    // No waypoints, fallback to idle
                    aiState = AIState::IDLE;
                    LOGW("NPC %u has no patrol waypoints, switching to IDLE", npcId);
                }
            }
            break;

        case AIState::FOLLOW_PLAYER:
            {
                float distToTarget = glm::distance(position, targetPosition);

                // Check if reached player
                if (distToTarget <= FOLLOW_PLAYER_DESIRED_DISTANCE) {
                    // Arrived at player, could switch to idle or stay at distance
                    LOGD("NPC %u reached follow target", npcId);
                }

                moveTowardsTarget(deltaTime);

                // Check for enemies while following
                if (shouldDetectEnemies()) {
                    // Enemy detection will be handled by combat system
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
    float distance = glm::length(direction);

    if (distance > 0.1f) {
        direction = glm::normalize(direction);
        position += direction * moveSpeed * 0.016f;  // Assume 60fps
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
    // Phase 2: Enhanced spell selection with multi-factor decision making

    // Check if no spells are available
    if (status.knownSpells.empty()) {
        LOGD("NPC %u has no known spells, using melee attack", npcId);
        return 0;  // No spells available, use melee
    }

    // Check spell casting cooldown
    if (lastSpellCastTime < 1.5f) {
        LOGD("NPC %u in spell cooldown (%.2f/1.5s)", npcId, lastSpellCastTime);
        return 0;  // Still in cooldown
    }

    // Prevent division by zero
    float hpPercent = (status.maxHealth > 0.0f)
        ? status.currentHealth / status.maxHealth
        : 1.0f;
    float manaPercent = (status.maxMana > 0.0f)
        ? status.currentMana / status.maxMana
        : 1.0f;

    LOGD("NPC %u spell selection: HP=%.1f%%, Mana=%.1f%%",
         npcId, hpPercent * 100.0f, manaPercent * 100.0f);

    // ============================================================
    // Priority 1: CRITICAL HP - Heal if available
    // ============================================================
    if (hpPercent < 0.3f) {
        uint32_t healSpell = findSpellByType(SpellType::RESTORATION);
        if (healSpell != 0 && canCastSpell(healSpell)) {
            LOGD("NPC %u priority: HEALING (HP critical at %.1f%%)",
                 npcId, hpPercent * 100.0f);
            return healSpell;
        }
    }

    // ============================================================
    // Priority 2: LOW MANA - Restore mana if available
    // ============================================================
    if (manaPercent < 0.2f && status.currentMana > 0) {
        uint32_t manaSpell = findSpellByType(SpellType::MANA_RESTORE);
        if (manaSpell != 0 && canCastSpell(manaSpell)) {
            LOGD("NPC %u priority: MANA RESTORE (mana low at %.1f%%)",
                 npcId, manaPercent * 100.0f);
            return manaSpell;
        }
    }

    // ============================================================
    // Priority 3: BUFF - Apply fortification if no buff active
    // ============================================================
    if (!hasBuffActive("Fortify") && hpPercent > 0.5f) {
        uint32_t buffSpell = findSpellByType(SpellType::FORTIFY);
        if (buffSpell != 0 && canCastSpell(buffSpell)) {
            LOGD("NPC %u priority: APPLYING BUFF (no fortify active)",
                 npcId);
            return buffSpell;
        }
    }

    // ============================================================
    // Priority 4: DAMAGE - Select most effective spell against enemy
    // ============================================================
    if (combatTarget && manaPercent > 0.3f) {
        uint32_t damageSpell = findMostEffectiveSpell(combatTarget);
        if (damageSpell != 0 && canCastSpell(damageSpell)) {
            LOGD("NPC %u priority: DAMAGE (selected spell %u)",
                 npcId, damageSpell);
            return damageSpell;
        }

        // Fallback: Try basic damage spell (Fireball)
        for (uint32_t spellId : status.knownSpells) {
            if (spellId == 2000 && canCastSpell(spellId)) {
                LOGD("NPC %u fallback: Using Fireball (damage)",
                     npcId);
                return 2000;
            }
        }
    }

    // ============================================================
    // No suitable spell found - use melee attack
    // ============================================================
    LOGD("NPC %u: No suitable spell available, using melee", npcId);
    return 0;  // Melee attack
}

bool NPC::canCastSpell(uint32_t spellId) const {
    // スペルIDが装備スペルに含まれているか確認
    auto it = std::find(status.equippedSpells.begin(),
                       status.equippedSpells.end(), spellId);
    return it != status.equippedSpells.end();
}

// Phase 1: Movement AI Helper Methods

void NPC::generateRandomWanderTarget() {
    // Generate random position within wanderRadius of current position
    // Using proper C++ random number generation

    // Initialize random generator (thread-safe with static local)
    static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
    static std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
    static std::uniform_real_distribution<float> distanceDist(0.0f, 1.0f);

    // Random angle (0 to 2π)
    float angle = angleDist(gen);

    // Random distance (0 to wanderRadius)
    float distance = wanderRadius * distanceDist(gen);

    // Calculate new target position
    float offsetX = distance * std::cos(angle);
    float offsetZ = distance * std::sin(angle);

    targetPosition = position + glm::vec3(offsetX, 0.0f, offsetZ);

    LOGD("NPC %u generated wander target: (%.1f, %.1f, %.1f)", npcId,
         targetPosition.x, targetPosition.y, targetPosition.z);
}

void NPC::moveTowardsTarget(float deltaTime) {
    glm::vec3 direction = targetPosition - position;
    float distance = glm::length(direction);

    // Only move if there's a distance to cover
    if (distance > 0.1f) {
        direction = glm::normalize(direction);

        // Move toward target with moveSpeed
        glm::vec3 movement = direction * moveSpeed * deltaTime;

        // Don't overshoot the target
        if (glm::length(movement) > distance) {
            position = targetPosition;
        } else {
            position += movement;
        }
    }
}

bool NPC::shouldDetectEnemies() {
    // Simple proximity-based enemy detection
    // Returns true if should check for enemies
    // In full implementation, this would query world/combat system
    // For now, always return true - actual detection happens in combat system
    return status.isAlive() && !inCombat;
}

// ============================================================
// Phase 2: Advanced Combat AI - Spell Selection
// ============================================================

uint32_t NPC::findSpellByType(SpellType type) {
    // Search for spell of given type in known spells
    for (uint32_t spellId : status.knownSpells) {
        // Spell ID ranges based on earlier implementation:
        // 2000 = Fireball (DAMAGE)
        // 2001 = Heal (RESTORATION)
        // 2002 = Restore Mana (MANA_RESTORE)
        // 2003 = Fortify (FORTIFY)

        SpellType spellType = SpellType::UNKNOWN;

        if (spellId == 2000) spellType = SpellType::DAMAGE;
        else if (spellId == 2001) spellType = SpellType::RESTORATION;
        else if (spellId == 2002) spellType = SpellType::MANA_RESTORE;
        else if (spellId == 2003) spellType = SpellType::FORTIFY;

        if (spellType == type) {
            LOGD("NPC %u found spell %u of type %d", npcId, spellId, static_cast<int>(type));
            return spellId;
        }
    }

    LOGD("NPC %u has no spell of type %d", npcId, static_cast<int>(type));
    return 0;
}

uint32_t NPC::findMostEffectiveSpell(std::shared_ptr<NPC> enemy) {
    if (!enemy) return 0;

    uint32_t bestSpell = 0;
    float bestScore = 0.0f;

    // Evaluate all known spells for effectiveness against enemy
    for (uint32_t spellId : status.knownSpells) {
        if (!canCastSpell(spellId)) continue;

        // Base damage (spell ID based estimation)
        float damage = 10.0f;
        if (spellId == 2000) damage = 45.0f;  // Fireball
        else if (spellId == 2001) damage = 0.0f;  // Heal (not offensive)
        else if (spellId == 2002) damage = 0.0f;  // Mana restore
        else if (spellId == 2003) damage = 0.0f;  // Fortify

        // Skip non-offensive spells for this calculation
        if (damage <= 0.0f) continue;

        // Enemy resistance based on Willpower attribute
        float enemyWillpower = enemy->status.attributes.count("Willpower") > 0 ?
                              enemy->status.attributes.at("Willpower") : 50.0f;
        float resistanceBonus = (enemyWillpower - 50.0f) / 10.0f;
        float effectiveDamage = std::max(0.0f, damage - resistanceBonus * 5.0f);

        // Mana efficiency: effective damage per mana point
        float manaCost = 50.0f;  // Default mana cost
        if (spellId == 2000) manaCost = 50.0f;  // Fireball

        // Prevent division by zero
        float efficiency = (manaCost > 0.0f)
            ? effectiveDamage / manaCost
            : 0.0f;

        LOGD("NPC %u evaluating spell %u: damage=%.1f, resistance=%.1f, efficiency=%.2f",
             npcId, spellId, effectiveDamage, resistanceBonus, efficiency);

        if (efficiency > bestScore) {
            bestScore = efficiency;
            bestSpell = spellId;
        }
    }

    if (bestSpell != 0) {
        LOGD("NPC %u selected most effective spell: %u (score: %.2f)",
             npcId, bestSpell, bestScore);
    }
    return bestSpell;
}

bool NPC::hasBuffActive(const std::string& buffName) const {
    // TODO: Implement buffer system
    // For now, always return false
    // In future, this would check active spell effects/buffs on the NPC
    LOGD("NPC %u checking for buff: %s", npcId, buffName.c_str());
    return false;
}

bool NPC::shouldFlee() const {
    // Determine if NPC should flee from combat

    // Calculate HP percentage (prevent division by zero)
    float hpPercent = (status.maxHealth > 0.0f)
        ? status.currentHealth / status.maxHealth
        : 1.0f;

    // Flee if HP is critically low (< 20%)
    if (hpPercent < 0.2f) {
        LOGD("NPC %u should flee: HP too low (%.1f%%)", npcId, hpPercent * 100.0f);
        return true;
    }

    // If no combat target, don't flee
    if (!combatTarget) return false;

    // Compare strength with enemy
    float ownStrength = status.attributes.count("Strength") > 0 ?
                       status.attributes.at("Strength") : 50.0f;
    float enemyStrength = combatTarget->status.attributes.count("Strength") > 0 ?
                         combatTarget->status.attributes.at("Strength") : 50.0f;
    float strengthDiff = ownStrength - enemyStrength;

    // Flee if significantly weaker (20+ point difference)
    if (strengthDiff < -20.0f && hpPercent < 0.4f) {
        LOGD("NPC %u should flee: Too weak vs enemy (diff: %.1f, HP: %.1f%%)",
             npcId, strengthDiff, hpPercent * 100.0f);
        return true;
    }

    return false;
}

// ============================================================
// Phase 4: Dialogue System
// ============================================================

bool NPC::startConversation() {
    // Note: Actual dialogue handling is managed by DialogueSystem
    // This just tracks conversation state in NPC
    if (inConversation) {
        LOGW("NPC %u is already in conversation", npcId);
        return false;
    }

    inConversation = true;
    currentDialogueNodeId = "greeting";  // Start at root node

    LOGI("NPC %u started conversation", npcId);
    return true;
}

void NPC::endConversation() {
    if (!inConversation) {
        LOGW("NPC %u is not in conversation", npcId);
        return;
    }

    inConversation = false;
    currentDialogueNodeId = "";

    LOGI("NPC %u ended conversation", npcId);
}

void NPC::updateModelMatrix() {
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}
