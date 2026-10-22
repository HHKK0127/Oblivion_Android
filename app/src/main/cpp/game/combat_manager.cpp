#include "combat_manager.h"
#include "player.h"
#include "spell_manager.h"
#include "fatigue_system.h"
#include "combat_formulas.h"
#include "../audio/audio_manager.h"
#include "../engine/imperial_weave.h"
#include <algorithm>
#include <cmath>
#include <random>

// ============================================================================
// Random number generator for combat rolls (function-local static to avoid
// static initialization order issues across translation units)
// ============================================================================
static std::mt19937& getRng() {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    return rng;
}

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

                // Apply block reduction (0x5474A0, RE_EXE_REPORT §56.1 /
                // s57_combat_full.md §3.1). This replaces the old fixed
                // 0.3 factor with the EXE's computed block amount.
                if (blocked) {
                    const CharacterStatus& def = combat.defender->status;
                    const auto blockIt = def.skills.find("Block");
                    const float blockSkill =
                        (blockIt != def.skills.end()) ? blockIt->second : 0.0f;
                    const auto luckIt = def.attributes.find("Luck");
                    const float luck =
                        (luckIt != def.attributes.end()) ? luckIt->second : 0.0f;
                    // equippedWeaponId 0 falls back to a default sword for
                    // attacking (see npcAttackPlayer); for the block style the
                    // original distinguishes weapon / hand-to-hand via the
                    // defender's weapon REF being present, so id 0 reads as
                    // the hand-to-hand term.
                    const bool hasWeapon = combat.defender->status.equippedWeaponId != 0;
                    const float reduction = oblivion::combat::blockAmount(
                        blockSkill, luck,
                        oblivion::fatigue::ratio(def.stamina, def.maxStamina),
                        /*isWeaponBlock=*/hasWeapon,
                        /*isHandToHand=*/!hasWeapon);
                    damage *= (1.0f - reduction);
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

                    // Knockback direction: defender pushed away from the
                    // attacker, horizontal only (Y up axis).
                    glm::vec3 hitDir = combat.defender->position - combat.attacker->position;
                    hitDir.y = 0.0f;

                    // febf0 段8: reflect damage. Defender's ReflectDamage AV
                    // (0x3b) may bounce a fraction back to the attacker.
                    // Player-reflector exclusion is internal to the formula.
                    const float reflected = oblivion::combat::applyReflectDamage(
                        damage, combat.defender->status, combat.attacker->status,
                        /*attackerIsPlayer=*/false);
                    if (reflected > 0.0f) {
                        combat.attacker->takeDamage(reflected);
                    }

                    applyDamage(combat.defender, damage, hitDir);

                    // febf0 段12: speak/stagger check on defender
                    oblivion::combat::applyHealthRatioEffects(damage,
                        combat.defender->status);

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

    // NPC -> player melee attacks (slice): hostile NPCs strike the player on a
    // fixed cooldown while within range. Approach/pursuit of the player and
    // ESM AIDT-driven hostility are future work. Player::takeDamage applies
    // the difficulty received factor f (RE_EXE_REPORT 51.13).
    if (player && npcManager) {
        glm::vec3 ppos = worldManager ? worldManager->getPlayerPosition() : glm::vec3(0.0f);
        constexpr float kPlayerAttackRange = 3.0f;
        for (const auto& npc : npcManager->getAllNPCs()) {
            if (!npc || !npc->hostileToPlayer || !npc->status.isAlive()) continue;
            npc->playerAttackTimer -= deltaTime;
            float dx = npc->position.x - ppos.x;
            float dz = npc->position.z - ppos.z;
            if (dx * dx + dz * dz > kPlayerAttackRange * kPlayerAttackRange) continue;
            if (npc->playerAttackTimer > 0.0f) continue;
            npc->playerAttackTimer = DAMAGE_CALCULATION_COOLDOWN;

            // NPC -> player magic attacks: select a combat spell, cast on player.
            // If the NPC has a spell and is in range, it casts instead of melee.
            uint32_t selectedSpell = npc->selectSpellForCombat();
            if (selectedSpell != 0 && spellManager) {
                npcCastSpellOnPlayer(npc, selectedSpell);
            } else {
                npcAttackPlayer(npc);
            }
        }
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

    // Fatigue damage multiplier (RE §17.11.1 Combat_CalcMeleeDamage chain):
    // fatigueRatio scales the whole offensive product via the 0x547F00 curve.
    // Exactly-full fatigue contributes 1.0 (no bonus); only sub-full fatigue
    // receives the curve value.
    const float fatigueRatio =
        oblivion::fatigue::ratio(attacker.stamina, attacker.maxStamina);
    const float fatigueMultiplier = oblivion::fatigue::meleeDamageMult(fatigueRatio);

    // Armor reduction (RE_EXE_REPORT §56.1 / s57_combat_full.md): the original
    // multiplies the running damage product by (1 - min(AR/100, 1.0)) at
    // 0x5FF7B5 / 0x5FF908, instead of subtracting a flat term.
    const bool applyArmor = !weapon.ignoresArmor;

    // Calculate final damage
    float totalDamage =
        (baseDamage + attributeBonus) * skillMultiplier * fatigueMultiplier;
    if (applyArmor) {
        totalDamage *= (1.0f - oblivion::combat::armorFactor(defender.armorRating));
    }

    // Ensure minimum damage
    if (totalDamage < 1.0f) {
        totalDamage = 1.0f;
    }

    return totalDamage;
}

float CombatManager::getDefenderArmorFactor(const CharacterStatus& defender) {
    return oblivion::combat::armorFactor(defender.armorRating);
}

void CombatManager::applyDamage(std::shared_ptr<NPC> target, float damage,
                                 const glm::vec3& hitDirection) {
    if (!target) return;

    target->takeDamage(damage, hitDirection);

    if (target->status.isAlive()) {
        // Trigger hit reaction animation on target; on death NPC::onDeath
        // already selected the DEATH animation instead.
        target->triggerHitReaction();
    }

    if (g_audioManager && damage > 0) {
        g_audioManager->playSound("magic/spell_hit");
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

    // Map ESM weapon type (0-6) to WeaponType enum
    static WeaponType esmWeaponTypeToWeaponType(uint16_t esmType) {
        switch (esmType) {
            case 0: return WeaponType::DAGGER;         // ShortBlade
            case 1: return WeaponType::SWORD_ONE_HAND; // LongBlade
            case 2: return WeaponType::MACE_ONE_HAND;  // Blunt1H
            case 3: return WeaponType::MACE_TWO_HAND;  // Blunt2H
            case 4: return WeaponType::SWORD_TWO_HAND; // Blade2H
            case 5: return WeaponType::BOW;            // Bow
            case 6: return WeaponType::STAFF;          // Staff
            default: return WeaponType::SWORD_ONE_HAND;
        }
    }

    void CombatManager::loadWeaponsFromESM(const oblivion::ESMManager& esmMgr) {
        const auto& weapons = esmMgr.getAllWeapons();
        size_t loaded = 0;

        for (const auto& w : weapons) {
            if (w.formID == 0) continue;
            if (weaponDatabase.count(w.formID) != 0) continue;

            WeaponType wt = esmWeaponTypeToWeaponType(w.weaponType);
            WeaponProperties props = getDefaultWeapon(wt);
            // Override base damage with ESM value
            props.baseDamage = static_cast<float>(w.damage);

            registerWeapon(w.formID, props);
            ++loaded;
        }

        LOGI("CombatManager: Loaded %zu weapons from ESM data (total database: %zu)",
             loaded, weaponDatabase.size());
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
    return dist(getRng()) < criticalChance;
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

    if (!rollCritical(blockChance)) return false;

    // Block fatigue cost (0x547590): 5 + incomingDamage·1.0 + BlockSkill·(-0.04).
    // NPC skill map has no "Block" entry yet, so the skill term stays 0.
    const float cost = oblivion::fatigue::blockCost(/*blockSkill=*/0.0f, incomingDamage);
    defender->status.stamina = oblivion::fatigue::clampValue(
        defender->status.stamina - cost, defender->status.maxStamina);
    return true;
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
    // Keep the attacker fatigue in sync with the live Player pool so the
    // damage-multiplier curve (0x547F00) reads the same value the attack and
    // sprint drains write to. The NPC-id-1 placeholder above does not track it.
    if (player) {
        playerStatus.stamina = player->stamina;
        playerStatus.maxStamina = player->maxStamina;
    }
    float damage = calculateDamage(playerStatus, target->status, weapon);
    // Difficulty scales the damage the player deals by 1/f (RE §50.1).
    damage *= Player::getDealtDamageScale();

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

    // febf0 段8: reflect damage. When the NPC defender has ReflectDamage AV
    // (0x3b), part of the player's damage bounces back. Player-reflector
    // exclusion: the formula skips when the attacker is the player (always
    // true in this path).
    const float reflected = oblivion::combat::applyReflectDamage(
        damage, target->status, playerStatus,
        /*attackerIsPlayer=*/true);

    applyDamage(target, damage);

    // febf0 段12: speak/stagger check
    oblivion::combat::applyHealthRatioEffects(damage, target->status);

    // Attack fatigue cost (0x547560): 8 + 0.1·damage for melee, zero for
    // bows and staves (weapon anim 4/5 are 0-cost in the original). Drains
    // the real Player stamina pool that updateStamina() recovers.
    if (player && !weapon.isRanged && weapon.type != WeaponType::STAFF) {
        const float cost =
            oblivion::fatigue::attackCost(damage, /*isPowerAttack=*/false);
        player->reduceStamina(cost);
    }

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

void CombatManager::npcAttackPlayer(std::shared_ptr<NPC> attacker) {
    if (!attacker || !player) return;

    // Per-NPC weapon lookup falls back to the default one-hand sword when the
    // id was never registered (same fallback initiateCombat() uses today).
    WeaponProperties weapon = getWeaponProperties(attacker->status.equippedWeaponId);

    // Build the defender status from the real Player; the NPC id 1 placeholder
    // used by playerAttack() never resolves to anything. armorRating is zeroed
    // because EquipmentEffectSystem (which would fill real player armor) has
    // its initialize call commented out in the renderer (renderer.cpp:2423),
    // so the armor factor inside calculateDamage() stays 0 (factor 0 -> the
    // multiplicative armor term contributes no reduction) for now.
    CharacterStatus defender{};
    defender.currentHealth = player->health;
    defender.maxHealth = player->maxHealth;
    defender.level = player->playerLevel;
    defender.armorRating = 0.0f;

    float damage = calculateDamage(attacker->status, defender, weapon);
    // Difficulty received factor f is applied here (RE_EXE_REPORT 51.13:
    // f = (d>0) ? d*M+1 : (d<0) ? 1/|d*M-1| : 1, and takeDamage multiplies
    // by it before the clamp).
    player->takeDamage(damage);

    // NPC attack fatigue cost (0x547560): same formula the player pays.
    // Bows and staves are 0-cost (weapon anim 4/5).
    if (!weapon.isRanged && weapon.type != WeaponType::STAFF) {
        const float cost =
            oblivion::fatigue::attackCost(damage, /*isPowerAttack=*/false);
        attacker->status.stamina = oblivion::fatigue::clampValue(
            attacker->status.stamina - cost, attacker->status.maxStamina);
    }

    constexpr uint32_t kPlayerEntityId = 1;  // ATK button convention
    emitCombatEvent(createCombatEvent(
        CombatEvent::Type::ATTACK_HIT,
        attacker->npcId,
        kPlayerEntityId,
        damage,
        attacker->position,
        weapon.type
    ));
    LOGI("NPC %s hits player for %.1f damage (player HP %.1f)",
         attacker->name.c_str(), damage, player->health);
}

void CombatManager::npcCastSpellOnPlayer(std::shared_ptr<NPC> caster, uint32_t spellId) {
    if (!spellManager || !player) return;

    auto spell = spellManager->getSpell(spellId);
    if (!spell || !spell->isAvailable(caster->status.currentMana)) return;

    if (!spellManager->consumeMana(caster->npcId, spell->manaCost)) return;

    // Cast fatigue cost
    caster->status.stamina = oblivion::fatigue::clampValue(
        caster->status.stamina - oblivion::fatigue::castCost(spell->manaCost),
        caster->status.maxStamina);

    // Apply spell effects directly to player, with difficulty scale f.
    // RE §50.1: NPC dealt-damage uses f (Player::getDifficultyScale).
    // Player::takeDamage does NOT re-multiply (it applies f on its own intake).
    const float difficultyScale = Player::getDifficultyScale();

    // Build defender stats from player for damage calculation
    CharacterStatus playerStatus;
    playerStatus.currentHealth = player->health;
    playerStatus.maxHealth = player->maxHealth;
    playerStatus.level = player->playerLevel;

    for (const auto& effect : spell->effects) {
        switch (effect.type) {
            case SpellEffectType::DAMAGE: {
                float dmg = spellManager->calculateSpellDamage(*spell, caster->status, playerStatus);
                dmg *= difficultyScale;
                player->takeDamage(dmg);
                break;
            }
            case SpellEffectType::HEAL:
                player->health += effect.magnitude;
                if (player->health > player->maxHealth) player->health = player->maxHealth;
                break;
            case SpellEffectType::RESTORE_MANA:
                player->magicka += effect.magnitude;
                if (player->magicka > player->maxMagicka) player->magicka = player->maxMagicka;
                break;
            case SpellEffectType::RESTORE_STAMINA:
                player->reduceStamina(-effect.magnitude);
                break;
            default:
                break;
        }
    }

    LOGI("NPC %s casts spell %u on player (HP %.1f)",
         caster->name.c_str(), spellId, player->health);
}

void CombatManager::playerCastSpell(uint32_t playerEntityId, uint32_t spellId, uint32_t targetNpcId) {
    if (!spellManager || !npcManager) return;

    // The NPC-based castSpell() applies the neutral damage scale, so a player
    // cast cannot delegate to it; run the mana/effect steps here instead.
    auto caster = npcManager->getNPC(playerEntityId);
    auto target = npcManager->getNPC(targetNpcId);
    auto spell = spellManager->getSpell(spellId);
    if (!caster || !target || !spell) {
        LOGW("Player cast failed: caster/target/spell missing (%u/%u/%u)",
             playerEntityId, targetNpcId, spellId);
        return;
    }
    if (!spell->isAvailable(caster->status.currentMana) ||
        !spellManager->consumeMana(playerEntityId, spell->manaCost)) {
        return;
    }
    // Difficulty scales the damage the player deals by 1/f (RE §50.1).
    spellManager->applySpellEffect(target, *spell, caster->status,
                                   Player::getDealtDamageScale());

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
        // Escape special characters in weapon name for JSON safety
        std::string escapedWeapon;
        for (char c : event.weaponName) {
            if (c == '"' || c == '\\') escapedWeapon += '\\';
            escapedWeapon += c;
        }
        payload += "\"weapon\":\"" + escapedWeapon + "\",";
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

std::vector<std::string> CombatManager::getActiveCombatsList() const {
    std::vector<std::string> result;
    for (const auto& pair : activeCombats) {
        const CombatInstance& combat = pair.second;
        if (combat.attacker && combat.defender) {
            std::string entry = combat.attacker->name + " (HP:" + std::to_string((int)combat.attacker->status.currentHealth) + ")";
            entry += " vs " + combat.defender->name + " (HP:" + std::to_string((int)combat.defender->status.currentHealth) + ")";
            entry += " [Dist:" + std::to_string((int)combat.distanceToTarget) + "]";
            result.push_back(entry);
        }
    }
    return result;
}
