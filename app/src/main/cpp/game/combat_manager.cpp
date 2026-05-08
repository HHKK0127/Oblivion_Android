#include "combat_manager.h"
#include "spell_manager.h"
#include "../system/cheat_manager.h"
#include <algorithm>
#include <cmath>

CombatManager::CombatManager()
    : worldManager(nullptr), npcManager(nullptr), spellManager(nullptr), cheatManager(nullptr) {
    LOGD("CombatManager created");
}

CombatManager::~CombatManager() {
    cleanup();
    LOGD("CombatManager destroyed");
}

bool CombatManager::initialize(WorldManager* wm, NpcManager* nm, class SpellManager* sm, class CheatManager* cm) {
    if (!wm || !nm) {
        LOGE("Cannot initialize CombatManager with null pointers");
        return false;
    }

    worldManager = wm;
    npcManager = nm;
    spellManager = sm;
    cheatManager = cm;
    LOGI("CombatManager initialized (SpellManager: %s, CheatManager: %s)",
         spellManager ? "available" : "not available",
         cheatManager ? "available" : "not available");
    return true;
}

void CombatManager::cleanup() {
    activeCombats.clear();
    worldManager = nullptr;
    npcManager = nullptr;
    cheatManager = nullptr;
    LOGD("CombatManager cleaned up");
}

void CombatManager::update(float deltaTime) {
    // Update active combats
    std::vector<uint32_t> endedCombats;

    for (auto& pair : activeCombats) {
        uint32_t defenderId = pair.first;
        CombatInstance& combat = pair.second;

        combat.lastAttackTime += deltaTime;
        combat.combatDuration += deltaTime;

        if (combat.defender && !combat.defender->status.isAlive()) {
            endedCombats.push_back(defenderId);
            continue;
        }

        if (combat.attacker && combat.defender &&
            combat.lastAttackTime >= DAMAGE_CALCULATION_COOLDOWN) {

            // NPC AI: スペル選択と発動
            if (spellManager && combat.attacker) {
                uint32_t selectedSpell = combat.attacker->selectSpellForCombat();
                if (selectedSpell != 0) {
                    // スペルキャスト試行
                    if (spellManager->castSpell(combat.attacker->npcId, selectedSpell,
                                               combat.defender->npcId)) {
                        combat.attacker->lastSpellCastTime = 0.0f;
                        combat.lastAttackTime = 0.0f;
                        LOGI("Combat Spell: %s cast spell (ID=%u) on %s",
                             combat.attacker->name.c_str(), selectedSpell,
                             combat.defender->name.c_str());
                        continue;  // スペル発動時は通常攻撃スキップ
                    }
                }
            }

            // 通常攻撃（スペルがない場合）
            float damage = calculateDamage(combat.attacker->status, combat.defender->status);
            if (damage > 0) {
                applyDamage(combat.defender, damage);
                LOGD("Combat: %s deals %.1f damage to %s",
                     combat.attacker->name.c_str(), damage, combat.defender->name.c_str());
                combat.lastAttackTime = 0.0f;
            }
        }

        // スペルキャスト間隔を更新
        if (combat.attacker) {
            combat.attacker->lastSpellCastTime += deltaTime;
        }
    }

    // End dead combats
    for (uint32_t defenderId : endedCombats) {
        endCombat(defenderId);
    }
}

void CombatManager::initiateCombat(std::shared_ptr<NPC> attacker, std::shared_ptr<NPC> defender) {
    if (!attacker || !defender) return;

    CombatInstance combat;
    combat.attacker = attacker;
    combat.defender = defender;
    combat.lastAttackTime = 0.0f;
    combat.combatDuration = 0.0f;

    activeCombats[defender->npcId] = combat;

    attacker->enterCombat(defender);
    defender->enterCombat(attacker);

    LOGI("Combat initiated: %s vs %s", attacker->name.c_str(), defender->name.c_str());
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

float CombatManager::calculateDamage(const CharacterStatus& attacker,
                                     const CharacterStatus& defender) {
    // Base damage from weapon
    float baseDamage = attacker.weaponDamage;

    // Strength bonus
    float strengthBonus = attacker.getAttributeBonus("Strength") * 2.0f;

    // Armor mitigation (apply ENEMY_WEAKNESS cheat here)
    float armorMitigation = getDefenderDamageMitigation(defender);

    // Calculate final damage
    float totalDamage = baseDamage + strengthBonus - armorMitigation;

    // Apply cheat effects
    if (cheatManager) {
        // CRITICAL_HIT_100: All attacks do 2x damage (critical hit)
        if (cheatManager->isCheatActive(CheatManager::CheatType::CRITICAL_HIT_100)) {
            totalDamage *= 2.0f;
        }

        // ONE_SHOT_KILL: Make damage extreme for one-shot kills
        if (cheatManager->isCheatActive(CheatManager::CheatType::ONE_SHOT_KILL)) {
            totalDamage = 99999.0f;  // One-hit kill
        }
    }

    // Ensure minimum damage
    if (totalDamage < 1.0f) {
        totalDamage = 1.0f;
    }

    return totalDamage;
}

float CombatManager::getDefenderDamageMitigation(const CharacterStatus& defender) {
    float mitigation = defender.armorRating * 0.5f;

    // ENEMY_WEAKNESS: Enemies take 4x damage (1/4 mitigation)
    if (cheatManager && cheatManager->isCheatActive(CheatManager::CheatType::ENEMY_WEAKNESS)) {
        mitigation *= 0.25f;  // Reduce mitigation to 25%
    }

    return mitigation;
}

void CombatManager::applyDamage(std::shared_ptr<NPC> target, float damage) {
    if (!target) return;

    target->takeDamage(damage);

    if (!target->status.isAlive()) {
        LOGI("NPC defeated: %s (HP: %.1f -> 0)", target->name.c_str(),
             target->status.maxHealth);
    }
}

void CombatManager::applyHeal(std::shared_ptr<NPC> target, float amount) {
    if (!target) return;

    target->heal(amount);
    LOGD("NPC healed: %s (+%.1f HP)", target->name.c_str(), amount);
}

std::shared_ptr<NPC> CombatManager::findNearestEnemy(std::shared_ptr<NPC> npc,
                                                      float detectionRadius) {
    if (!npc || !npcManager) return nullptr;

    auto allNpcs = npcManager->getAllNPCs();
    std::shared_ptr<NPC> nearestEnemy = nullptr;
    float minDistanceSq = detectionRadius * detectionRadius;  // Compare squared distances

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
            LOGD("  %s (HP:%.1f) vs %s (HP:%.1f)",
                 combat.attacker->name.c_str(), combat.attacker->status.currentHealth,
                 combat.defender->name.c_str(), combat.defender->status.currentHealth);
        }
    }
    LOGD("==========================================");
}
