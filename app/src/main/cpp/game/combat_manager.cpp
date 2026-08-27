#include "combat_manager.h"
#include "spell_manager.h"
#include "../audio/audio_manager.h"
#include "../engine/imperial_weave.h"
#include <algorithm>
#include <cmath>
#include <random>

// ============================================================================
// Random number generator for combat rolls
// ============================================================================
static std::random_device g_rd;
static std::mt19937 g_rng(g_rd());

// ============================================================================
// Helper: CombatEvent type to string
// ============================================================================
static std::string combatEventToString(CombatEvent::Type type) {
     switch (type) {
         case CombatEvent::Type::ATTACK_START: return "ATTACK_START";
         case CombatEvent::Type::ATTACK_HIT: return "ATTACK_HIT";
         case CombatEvent::Type::ATTACK_MISS: return "ATTACK_MISS";
         case CombatEvent::Type::CRITICAL_HIT: return "CRITICAL_HIT";
         case CombatEvent::Type::BLOCK: return "BLOCK";
         case CombatEvent::Type::PARRY: return "PARRY";
         case CombatEvent::Type::DODGE: return "DODGE";
         case CombatEvent::Type::DEATH: return "DEATH";
         case CombatEvent::Type::BLEED_APPLIED: return "BLEED_APPLIED";
         case CombatEvent::Type::MAGIC_CAST: return "MAGIC_CAST";
         default: return "UNKNOWN";
     }
}

static std::string weaponTypeToAudioKey(WeaponType type) {
    switch (type) {
        case WeaponType::SWORD_ONE_HAND: return "blade";
        case WeaponType::SWORD_TWO_HAND: return "blade";
        case WeaponType::DAGGER:         return "blade";
        case WeaponType::AXE_ONE_HAND:   return "axe";
        case WeaponType::AXE_TWO_HAND:   return "axe";
        case WeaponType::MACE_ONE_HAND:  return "blunt";
        case WeaponType::MACE_TWO_HAND:  return "blunt";
        case WeaponType::BOW:            return "bow";
        case WeaponType::STAFF:          return "staff";
        default:                         return "unarmed";
    }
}

// ============================================================================
// Helper: Create CombatEvent with default values
// ============================================================================
static CombatEvent createCombatEvent(
    CombatEvent::Type type,
    uint32_t attackerId,
    uint32_t defenderId,
    float damage,
    const glm::vec3& hitPosition,
    WeaponType weaponType,
    float timestamp = 0.0f,
    const std::string& weaponName = "",
    bool isCritical = false,
    bool isBlocked = false
) {
    CombatEvent event;
    event.type = type;
    event.attackerId = attackerId;
    event.defenderId = defenderId;
    event.targetId = defenderId;
    event.damage = damage;
    event.timestamp = timestamp;
    event.hitPosition = hitPosition;
    event.weaponType = weaponType;
    event.weaponName = weaponName;
    event.isCritical = isCritical;
    event.isBlocked = isBlocked;
    return event;
}

CombatManager::CombatManager()
    : worldManager(nullptr), npcManager(nullptr), spellManager(nullptr),
      cheatManager(nullptr), navMeshManager(nullptr) {
    LOGD("CombatManager created");
}

CombatManager::~CombatManager() {
    cleanup();
    LOGD("CombatManager destroyed");
}

bool CombatManager::initialize(WorldManager* wm, NpcManager* nm, class SpellManager* sm,
                               class CheatManager* cm, oblivion::NavMeshManager* nvm,
                               class weave::EventBus* eb) {
    if (!wm || !nm) {
        LOGE("Cannot initialize CombatManager with null pointers");
        return false;
    }

    worldManager = wm;
    npcManager = nm;
    spellManager = sm;
    cheatManager = cm;
    navMeshManager = nvm;
    eventBus = eb;

    // Register default weapon types
    registerWeapon(0, getDefaultWeapon(WeaponType::SWORD_ONE_HAND));

    LOGI("CombatManager initialized (SpellManager: %s, CheatManager: %s, NavMesh: %s, EventBus: %s)",
         spellManager ? "available" : "not available",
         cheatManager ? "available" : "not available",
         navMeshManager ? "available" : "not available",
         eventBus ? "connected" : "not connected");
    return true;
}

void CombatManager::cleanup() {
    activeCombats.clear();
    activeHitboxes.clear();
    eventQueue.clear();
    weaponDatabase.clear();
    worldManager = nullptr;
    npcManager = nullptr;
    cheatManager = nullptr;
    navMeshManager = nullptr;
    LOGD("CombatManager cleaned up");
}

// ============================================================================
// Update - Main combat loop
// ============================================================================

void CombatManager::update(float deltaTime) {
    // Update hitboxes
    updateHitboxes(deltaTime);

    // Update active combats
    std::vector<uint32_t> endedCombats;

    for (auto& pair : activeCombats) {
        uint32_t defenderId = pair.first;
        CombatInstance& combat = pair.second;

        combat.lastAttackTime += deltaTime;
        combat.combatDuration += deltaTime;

        // Check if defender is dead
        if (combat.defender && !combat.defender->status.isAlive()) {
            emitCombatEvent(createCombatEvent(
                CombatEvent::Type::DEATH,
                combat.attacker ? combat.attacker->npcId : 0,
                defenderId,
                0.0f,
                combat.defender->position,
                combat.attackerWeapon.type
            ));
            endedCombats.push_back(defenderId);
            continue;
        }

        if (combat.attacker && combat.defender) {
            // Calculate distance to target
            glm::vec3 diff = combat.defender->position - combat.attacker->position;
            float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            combat.distanceToTarget = std::sqrt(distSq);
            combat.isInAttackRange = combat.distanceToTarget <= combat.attackerWeapon.range;

            // Move toward enemy if too far
            if (!combat.isInAttackRange && navMeshManager) {
                combat.attacker->moveTo(combat.defender->position, navMeshManager);
            }

            // Attack when in range and cooldown ready
            float attackCooldown = DAMAGE_CALCULATION_COOLDOWN / combat.attackerWeapon.attackSpeed;
            if (combat.isInAttackRange && combat.lastAttackTime >= attackCooldown) {

                // Emit attack start event
                emitCombatEvent(createCombatEvent(
                    CombatEvent::Type::ATTACK_START,
                    combat.attacker->npcId,
                    defenderId,
                    0.0f,
                    combat.attacker->position,
                    combat.attackerWeapon.type
                ));

                // Check for parry
                if (attemptParry(combat.defender, combat.lastAttackTime)) {
                    emitCombatEvent(createCombatEvent(
                        CombatEvent::Type::PARRY,
                        defenderId,
                        combat.attacker->npcId,
                        0.0f,
                        combat.defender->position,
                        combat.attackerWeapon.type
                    ));
                    combat.lastAttackTime = 0.0f;
                    continue;
                }

                // Check for dodge
                if (attemptDodge(combat.defender)) {
                    emitCombatEvent(createCombatEvent(
                        CombatEvent::Type::DODGE,
                        defenderId,
                        combat.attacker->npcId,
                        0.0f,
                        combat.defender->position,
                        combat.attackerWeapon.type
                    ));
                    combat.lastAttackTime = 0.0f;
                    continue;
                }

                // Check for block
                bool blocked = attemptBlock(combat.defender, 0.0f);

                // NPC AI: Spell selection and casting
                if (spellManager && combat.attacker) {
                    uint32_t selectedSpell = combat.attacker->selectSpellForCombat();
                    if (selectedSpell != 0) {
                        if (spellManager->castSpell(combat.attacker->npcId, selectedSpell,
                                                   combat.defender->npcId)) {
                            combat.attacker->lastSpellCastTime = 0.0f;
                            combat.lastAttackTime = 0.0f;
                            LOGI("Combat Spell: %s cast spell (ID=%u) on %s",
                                 combat.attacker->name.c_str(), selectedSpell,
                                 combat.defender->name.c_str());
                            continue;
                        }
                    }
                }

                // Normal attack
                float damage = calculateDamage(combat.attacker->status,
                                              combat.defender->status,
                                              combat.attackerWeapon);

                // Apply block reduction
                if (blocked) {
                    damage *= 0.3f;
                    emitCombatEvent(createCombatEvent(
                        CombatEvent::Type::BLOCK,
                        defenderId,
                        combat.attacker->npcId,
                        damage,
                        combat.defender->position,
                        combat.attackerWeapon.type,
                        0.0f,
                        "",
                        false,
                        true
                    ));
                }

                // Critical hit check
                bool isCritical = rollCritical(combat.attackerWeapon.criticalChance);
                if (isCritical) {
                    damage = applyCriticalDamage(damage, combat.attackerWeapon.criticalMultiplier);
                    emitCombatEvent(createCombatEvent(
                        CombatEvent::Type::CRITICAL_HIT,
                        combat.attacker->npcId,
                        defenderId,
                        damage,
                        combat.defender->position,
                        combat.attackerWeapon.type,
                        0.0f,
                        "",
                        true,
                        false
                    ));
                    LOGI("CRITICAL HIT! %s deals %.1f damage to %s",
                         combat.attacker->name.c_str(), damage, combat.defender->name.c_str());
                }

                if (damage > 0) {
                    // Create hitbox at hit position
                    glm::vec3 hitPos = (combat.attacker->position + combat.defender->position) * 0.5f;
                    createHitbox(hitPos, glm::vec3(0.5f, 0.5f, 0.5f), damage, combat.attacker->npcId);

                    applyDamage(combat.defender, damage);

                    emitCombatEvent(createCombatEvent(
                        CombatEvent::Type::ATTACK_HIT,
                        combat.attacker->npcId,
                        defenderId,
                        damage,
                        hitPos,
                        combat.attackerWeapon.type
                    ));

                    LOGD("Combat: %s deals %.1f damage to %s (weapon: %d)",
                         combat.attacker->name.c_str(), damage,
                         combat.defender->name.c_str(), static_cast<int>(combat.attackerWeapon.type));
                }

                // Apply bleed for axe weapons
                if (combat.attackerWeapon.bleedChance > 0.0f &&
                    rollCritical(combat.attackerWeapon.bleedChance)) {
                    applyBleed(combat.defender, BLEED_DURATION, BLEED_DAMAGE_PER_SEC);
                    emitCombatEvent(createCombatEvent(
                        CombatEvent::Type::BLEED_APPLIED,
                        combat.attacker->npcId,
                        defenderId,
                        BLEED_DAMAGE_PER_SEC,
                        combat.defender->position,
                        combat.attackerWeapon.type
                    ));
                }

                combat.lastAttackTime = 0.0f;
            }
        }

        // Update spell cast cooldown
        if (combat.attacker) {
            combat.attacker->lastSpellCastTime += deltaTime;
        }
    }

    // End dead combats
    for (uint32_t defenderId : endedCombats) {
        endCombat(defenderId);
    }
}

// ============================================================================
// Combat lifecycle
// ============================================================================

void CombatManager::initiateCombat(std::shared_ptr<NPC> attacker, std::shared_ptr<NPC> defender) {
    if (!attacker || !defender) return;

    CombatInstance combat;
    combat.attacker = attacker;
    combat.defender = defender;
    combat.lastAttackTime = 0.0f;
    combat.combatDuration = 0.0f;
    combat.attackerWeapon = getDefaultWeapon(WeaponType::SWORD_ONE_HAND);

    activeCombats[defender->npcId] = combat;

    attacker->enterCombat(defender);
    defender->enterCombat(attacker);

    emitCombatEvent(createCombatEvent(
        CombatEvent::Type::ATTACK_START,
        attacker->npcId,
        defender->npcId,
        0.0f,
        attacker->position,
        combat.attackerWeapon.type
    ));

    LOGI("Combat initiated: %s vs %s", attacker->name.c_str(), defender->name.c_str());

    if (g_audioManager) {
        g_audioManager->playSound("combat/blade_equip");
    }
}

void CombatManager::endCombat(uint32_t defenderId) {
    auto it = activeCombats.find(defenderId);
    if (it != activeCombats.end()) {
        CombatInstance& combat = it->second;

        if (combat.attacker) {
            combat.attacker->exitCombat();
        }
        if (combat.defender) {
            combat.defender->exitCombat();
        }

        LOGI("Combat ended between %s and %s (duration: %.1f sec)",
             combat.attacker ? combat.attacker->name.c_str() : "Unknown",
             combat.defender ? combat.defender->name.c_str() : "Unknown",
             combat.combatDuration);

        activeCombats.erase(it);
    }
}

// ============================================================================
// Damage calculation (enhanced with weapon types)
// ============================================================================

float CombatManager::calculateDamage(const CharacterStatus& attacker,
                                     const CharacterStatus& defender,
                                     const WeaponProperties& weapon) {
    // Base damage from weapon
    float baseDamage = weapon.baseDamage > 0 ? weapon.baseDamage : attacker.weaponDamage;

    // Strength bonus (melee) or Agility bonus (ranged)
    float attributeBonus = 0.0f;
    if (weapon.isRanged) {
        attributeBonus = attacker.getAttributeBonus("Agility") * 1.5f;
    } else {
        attributeBonus = attacker.getAttributeBonus("Strength") * 2.0f;
    }

    // Skill multiplier
    float skillMultiplier = 1.0f;

    // Armor mitigation
    float armorMitigation = 0.0f;
    if (!weapon.ignoresArmor) {
        armorMitigation = getDefenderDamageMitigation(defender);
    }

    // Calculate final damage
    float totalDamage = (baseDamage + attributeBonus) * skillMultiplier - armorMitigation;

    // Ensure minimum damage
    if (totalDamage < 1.0f) {
        totalDamage = 1.0f;
    }

    return totalDamage;
}

float CombatManager::getDefenderDamageMitigation(const CharacterStatus& defender) {
    float mitigation = defender.armorRating * 0.5f;
    return mitigation;
}

void CombatManager::applyDamage(std::shared_ptr<NPC> target, float damage) {
    if (!target) return;

    target->takeDamage(damage);

    // Trigger hit reaction animation on target
    target->triggerHitReaction();

    if (g_audioManager && damage > 0) {
        g_audioManager->playSound("magic/spell_hit");
    }

    if (!target->status.isAlive()) {
        // Trigger death animation
        target->triggerDeath();
        LOGI("NPC defeated: %s (HP: %.1f -> 0)", target->name.c_str(),
             target->status.maxHealth);
    }
}

void CombatManager::applyHeal(std::shared_ptr<NPC> target, float amount) {
    if (!target) return;

    target->heal(amount);
    LOGD("NPC healed: %s (+%.1f HP)", target->name.c_str(), amount);
}

// ============================================================================
// Weapon system
// ============================================================================

void CombatManager::registerWeapon(uint32_t weaponId, const WeaponProperties& props) {
    weaponDatabase[weaponId] = props;
    LOGD("Weapon registered: ID=%u, Type=%d, Damage=%.1f",
         weaponId, static_cast<int>(props.type), props.baseDamage);
}

WeaponProperties CombatManager::getWeaponProperties(uint32_t weaponId) const {
    auto it = weaponDatabase.find(weaponId);
    if (it != weaponDatabase.end()) {
        return it->second;
    }
    return getDefaultWeapon(WeaponType::SWORD_ONE_HAND);
}

WeaponProperties CombatManager::getDefaultWeapon(WeaponType type) const {
    WeaponProperties props;
    props.type = type;

    switch (type) {
        case WeaponType::SWORD_ONE_HAND:
            props.baseDamage = 10.0f;
            props.attackSpeed = 1.2f;
            props.range = 3.0f;
            props.criticalChance = 0.06f;
            props.criticalMultiplier = 2.0f;
            props.staminaCost = 8.0f;
            break;

        case WeaponType::SWORD_TWO_HAND:
            props.baseDamage = 18.0f;
            props.attackSpeed = 0.8f;
            props.range = 4.0f;
            props.criticalChance = 0.05f;
            props.criticalMultiplier = 2.5f;
            props.staminaCost = 15.0f;
            break;

        case WeaponType::AXE_ONE_HAND:
            props.baseDamage = 12.0f;
            props.attackSpeed = 1.0f;
            props.range = 3.0f;
            props.criticalChance = 0.04f;
            props.criticalMultiplier = 2.0f;
            props.staminaCost = 10.0f;
            props.bleedChance = 0.15f;
            break;

        case WeaponType::AXE_TWO_HAND:
            props.baseDamage = 22.0f;
            props.attackSpeed = 0.7f;
            props.range = 4.0f;
            props.criticalChance = 0.04f;
            props.criticalMultiplier = 2.5f;
            props.staminaCost = 18.0f;
            props.bleedChance = 0.25f;
            break;

        case WeaponType::MACE_ONE_HAND:
            props.baseDamage = 11.0f;
            props.attackSpeed = 0.9f;
            props.range = 3.0f;
            props.criticalChance = 0.03f;
            props.criticalMultiplier = 2.0f;
            props.staminaCost = 12.0f;
            props.ignoresArmor = true;
            break;

        case WeaponType::MACE_TWO_HAND:
            props.baseDamage = 20.0f;
            props.attackSpeed = 0.6f;
            props.range = 4.0f;
            props.criticalChance = 0.03f;
            props.criticalMultiplier = 2.5f;
            props.staminaCost = 20.0f;
            props.ignoresArmor = true;
            break;

        case WeaponType::DAGGER:
            props.baseDamage = 6.0f;
            props.attackSpeed = 1.8f;
            props.range = 2.0f;
            props.criticalChance = 0.12f;
            props.criticalMultiplier = 3.0f;
            props.staminaCost = 5.0f;
            break;

        case WeaponType::BOW:
            props.baseDamage = 14.0f;
            props.attackSpeed = 0.6f;
            props.range = 30.0f;
            props.criticalChance = 0.07f;
            props.criticalMultiplier = 2.0f;
            props.staminaCost = 10.0f;
            props.isRanged = true;
            break;

        case WeaponType::STAFF:
            props.baseDamage = 8.0f;
            props.attackSpeed = 0.8f;
            props.range = 5.0f;
            props.criticalChance = 0.03f;
            props.criticalMultiplier = 1.5f;
            props.staminaCost = 5.0f;
            break;

        default:
            props.baseDamage = 5.0f;
            props.attackSpeed = 1.0f;
            props.range = 2.0f;
            break;
    }

    return props;
}

// ============================================================================
// Hitbox system
// ============================================================================

void CombatManager::createHitbox(const glm::vec3& position, const glm::vec3& halfExtents,
                                float damage, uint32_t ownerEntityId) {
    Hitbox hitbox;
    hitbox.center = position;
    hitbox.halfExtents = halfExtents;
    hitbox.damage = damage;
    hitbox.ownerEntityId = ownerEntityId;
    hitbox.lifetime = HITBOX_LIFETIME;
    hitbox.elapsed = 0.0f;
    hitbox.isActive = true;

    activeHitboxes.push_back(hitbox);
}

void CombatManager::updateHitboxes(float deltaTime) {
    // Update and remove expired hitboxes
    for (auto it = activeHitboxes.begin(); it != activeHitboxes.end(); ) {
        it->elapsed += deltaTime;
        if (it->elapsed >= it->lifetime) {
            it = activeHitboxes.erase(it);
        } else {
            ++it;
        }
    }
}

bool CombatManager::checkHitboxCollision(const Hitbox& hitbox, std::shared_ptr<NPC> target) const {
    if (!target || !hitbox.isActive) return false;

    // AABB collision check
    glm::vec3 diff = target->position - hitbox.center;
    return (std::abs(diff.x) <= hitbox.halfExtents.x + 0.5f) &&
           (std::abs(diff.y) <= hitbox.halfExtents.y + 1.0f) &&
           (std::abs(diff.z) <= hitbox.halfExtents.z + 0.5f);
}

// ============================================================================
// Critical hit system
// ============================================================================

bool CombatManager::rollCritical(float criticalChance) const {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(g_rng) < criticalChance;
}

float CombatManager::applyCriticalDamage(float damage, float criticalMultiplier) const {
    return damage * criticalMultiplier;
}

// ============================================================================
// Block/Parry system
// ============================================================================

bool CombatManager::attemptParry(std::shared_ptr<NPC> defender, float attackTime) {
    if (!defender) return false;

    // Parry chance based on Agility (simplified)
    float parryChance = defender->status.getAttributeBonus("Agility") * 0.01f;
    parryChance = std::min(parryChance, 0.25f); // Max 25% parry chance

    return rollCritical(parryChance);
}

bool CombatManager::attemptBlock(std::shared_ptr<NPC> defender, float incomingDamage) {
    if (!defender) return false;

    // Block chance based on Endurance
    float blockChance = defender->status.getAttributeBonus("Endurance") * 0.01f;
    blockChance = std::min(blockChance, 0.30f); // Max 30% block chance

    return rollCritical(blockChance);
}

bool CombatManager::attemptDodge(std::shared_ptr<NPC> defender) {
    if (!defender) return false;

    // Dodge chance based on Speed
    float dodgeChance = defender->status.getAttributeBonus("Speed") * 0.008f;
    dodgeChance = std::min(dodgeChance, 0.20f); // Max 20% dodge chance

    return rollCritical(dodgeChance);
}

// ============================================================================
// Bleed system
// ============================================================================

void CombatManager::applyBleed(std::shared_ptr<NPC> target, float duration, float damagePerSec) {
    if (!target) return;

    // Apply bleed as damage over time (simplified - just apply immediate damage for now)
    float totalBleedDamage = duration * damagePerSec;
    target->takeDamage(totalBleedDamage * 0.1f); // Apply 10% immediately

    LOGD("Bleed applied to %s: %.1f damage over %.1f sec",
         target->name.c_str(), totalBleedDamage, duration);
}

// ============================================================================
// Player combat
// ============================================================================

void CombatManager::playerAttack(uint32_t playerEntityId, uint32_t targetNpcId, uint32_t weaponId) {
    if (!npcManager) return;

    auto target = npcManager->getNPC(targetNpcId);
    if (!target || !target->status.isAlive()) return;

    WeaponProperties weapon = getWeaponProperties(weaponId);

    // Create hitbox for player attack
    glm::vec3 hitPos = target->position;
    createHitbox(hitPos, glm::vec3(weapon.range * 0.5f, 1.0f, weapon.range * 0.5f),
                weapon.baseDamage, playerEntityId);

    // Calculate and apply damage
    // FIX: Get actual player status from NPC manager (player is NPC ID 1)
    auto playerNpc = npcManager->getNPC(1);
    CharacterStatus playerStatus;
    if (playerNpc) {
        playerStatus = playerNpc->status;
    }
    playerStatus.weaponDamage = weapon.baseDamage;
    float damage = calculateDamage(playerStatus, target->status, weapon);

    // Critical hit
    if (rollCritical(weapon.criticalChance)) {
        damage = applyCriticalDamage(damage, weapon.criticalMultiplier);
        emitCombatEvent(createCombatEvent(
            CombatEvent::Type::CRITICAL_HIT,
            playerEntityId,
            targetNpcId,
            damage,
            hitPos,
            weapon.type,
            0.0f,
            "",
            true,
            false
        ));
    }

    applyDamage(target, damage);

    emitCombatEvent(createCombatEvent(
        CombatEvent::Type::ATTACK_HIT,
        playerEntityId,
        targetNpcId,
        damage,
        hitPos,
        weapon.type
    ));

    LOGI("Player attacks %s for %.1f damage", target->name.c_str(), damage);
}

void CombatManager::playerCastSpell(uint32_t playerEntityId, uint32_t spellId, uint32_t targetNpcId) {
    if (!spellManager) return;

    spellManager->castSpell(playerEntityId, spellId, targetNpcId);

    LOGI("Player casts spell %u on target %u", spellId, targetNpcId);
}

// ============================================================================
// Event system (Imperial Weave integration)
// ============================================================================

void CombatManager::emitCombatEvent(const CombatEvent& event) {
    eventQueue.push_back(event);

    // Also emit to Imperial Weave EventBus if available
    if (eventBus) {
        weave::Event weaveEvent;
        weaveEvent.type = "COMBAT_" + combatEventToString(event.type);
        weaveEvent.sender = event.attackerId;
        weaveEvent.time = event.timestamp;

        // Build payload JSON
        std::string payload = "{";
        payload += "\"type\":\"" + combatEventToString(event.type) + "\",";
        payload += "\"attacker\":" + std::to_string(event.attackerId) + ",";
        payload += "\"target\":" + std::to_string(event.targetId) + ",";
        payload += "\"damage\":" + std::to_string(event.damage) + ",";
        payload += "\"weapon\":\"" + event.weaponName + "\",";
        payload += "\"weaponType\":\"" + weaponTypeToAudioKey(event.weaponType) + "\",";
        payload += "\"isCritical\":" + std::string(event.isCritical ? "true" : "false") + ",";
        payload += "\"isBlocked\":" + std::string(event.isBlocked ? "true" : "false");
        payload += "}";
        weaveEvent.payload = payload;

        eventBus->emit(weaveEvent);
    }
}

std::vector<CombatEvent> CombatManager::consumeEvents() {
    std::vector<CombatEvent> events;
    events.swap(eventQueue);
    return events;
}

// ============================================================================
// Utility
// ============================================================================

std::shared_ptr<NPC> CombatManager::findNearestEnemy(std::shared_ptr<NPC> npc,
                                                      float detectionRadius) {
    if (!npc || !npcManager) return nullptr;

    auto allNpcs = npcManager->getAllNPCs();
    std::shared_ptr<NPC> nearestEnemy = nullptr;
    float minDistanceSq = detectionRadius * detectionRadius;

    for (const auto& other : allNpcs) {
        if (!other || other->npcId == npc->npcId) continue;

        glm::vec3 diff = other->position - npc->position;
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distanceSq < minDistanceSq && other->status.isAlive()) {
            minDistanceSq = distanceSq;
            nearestEnemy = other;
        }
    }

    return nearestEnemy;
}

// ============================================================================
// Find nearest enemy to player
// ============================================================================
std::shared_ptr<NPC> CombatManager::findNearestEnemyToPlayer(const glm::vec3& playerPos, float detectionRadius) {
    if (!npcManager) return nullptr;

    auto allNpcs = npcManager->getAllNPCs();
    std::shared_ptr<NPC> nearestEnemy = nullptr;
    float minDistanceSq = detectionRadius * detectionRadius;

    for (const auto& npc : allNpcs) {
        if (!npc || !npc->status.isAlive()) continue;

        glm::vec3 diff = npc->position - playerPos;
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distanceSq < minDistanceSq) {
            minDistanceSq = distanceSq;
            nearestEnemy = npc;
        }
    }

    return nearestEnemy;
}

bool CombatManager::isInCombat(uint32_t npcId) const {
    return activeCombats.find(npcId) != activeCombats.end();
}

const CombatInstance* CombatManager::getCombat(uint32_t defenderId) const {
    auto it = activeCombats.find(defenderId);
    if (it != activeCombats.end()) {
        return &it->second;
    }
    return nullptr;
}

void CombatManager::clearCombats() {
    activeCombats.clear();
    LOGD("All combats cleared");
}

void CombatManager::logCombatStatus() const {
    LOGD("========== Combat Manager Status ==========");
    LOGD("Active combats: %zu", activeCombats.size());
    for (const auto& pair : activeCombats) {
        const CombatInstance& combat = pair.second;
        if (combat.attacker && combat.defender) {
            LOGD("  %s (HP:%.1f) vs %s (HP:%.1f) [Dist: %.1f, Range: %s]",
                 combat.attacker->name.c_str(), combat.attacker->status.currentHealth,
                 combat.defender->name.c_str(), combat.defender->status.currentHealth,
                 combat.distanceToTarget,
                 combat.isInAttackRange ? "IN" : "OUT");
        }
    }
    LOGD("==========================================");
}
