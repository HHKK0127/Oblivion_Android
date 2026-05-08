#include "spell_manager.h"
#include "../system/cheat_manager.h"
#include <algorithm>
#include <cmath>

SpellManager::SpellManager()
    : npcManager(nullptr), cheatManager(nullptr), nextSpellId(2000) {
    LOGD("SpellManager created");
}

SpellManager::~SpellManager() {
    cleanup();
    LOGD("SpellManager destroyed");
}

bool SpellManager::initialize(NpcManager* nm, class CheatManager* cm) {
    if (!nm) {
        LOGE("Cannot initialize SpellManager with null NpcManager");
        return false;
    }

    npcManager = nm;
    cheatManager = cm;
    LOGI("SpellManager initialized with NpcManager (CheatManager: %s)",
         cheatManager ? "available" : "not available");
    return true;
}

void SpellManager::cleanup() {
    spells.clear();
    npcManager = nullptr;
    cheatManager = nullptr;
    LOGD("SpellManager cleaned up");
}

void SpellManager::update(float deltaTime) {
    // スペルの定期的な処理はここに追加可能
    // 例：継続的な効果の管理など
}

uint32_t SpellManager::createSpell(const std::string& name, const std::string& nameJa,
                                   MagicSchool school, float manaCost, float baseDamage) {
    uint32_t spellId = nextSpellId++;
    auto spell = std::make_shared<Spell>(spellId, name, nameJa, school, manaCost, baseDamage);

    spells[spellId] = spell;

    LOGD("Spell created: ID=%u, Name=%s(%s), School=%s, ManaCost=%.1f, Damage=%.1f",
         spellId, name.c_str(), nameJa.c_str(), spell->getSchoolName().c_str(),
         manaCost, baseDamage);
    return spellId;
}

std::shared_ptr<Spell> SpellManager::getSpell(uint32_t spellId) const {
    auto it = spells.find(spellId);
    if (it == spells.end()) {
        return nullptr;
    }
    return it->second;
}

void SpellManager::addEffectToSpell(uint32_t spellId, const SpellEffect& effect) {
    auto it = spells.find(spellId);
    if (it == spells.end()) {
        LOGW("Spell ID %u not found", spellId);
        return;
    }

    it->second->effects.push_back(effect);
    LOGD("Effect added to spell %u", spellId);
}

void SpellManager::teachSpellToNpc(uint32_t npcId, uint32_t spellId) {
    if (!npcManager) return;

    auto npc = npcManager->getNPC(npcId);
    if (!npc) {
        LOGW("NPC ID %u not found", npcId);
        return;
    }

    auto it = std::find(npc->status.knownSpells.begin(),
                       npc->status.knownSpells.end(), spellId);
    if (it == npc->status.knownSpells.end()) {
        npc->status.knownSpells.push_back(spellId);
        LOGI("Spell %u taught to NPC %u", spellId, npcId);
    }
}

void SpellManager::equipSpellToNpc(uint32_t npcId, uint32_t spellId) {
    if (!npcManager) return;

    auto npc = npcManager->getNPC(npcId);
    if (!npc) {
        LOGW("NPC ID %u not found", npcId);
        return;
    }

    // スペルが既知かどうか確認
    auto it = std::find(npc->status.knownSpells.begin(),
                       npc->status.knownSpells.end(), spellId);
    if (it == npc->status.knownSpells.end()) {
        LOGW("NPC %u does not know spell %u", npcId, spellId);
        return;
    }

    // 装備スペルに追加
    auto equipIt = std::find(npc->status.equippedSpells.begin(),
                            npc->status.equippedSpells.end(), spellId);
    if (equipIt == npc->status.equippedSpells.end()) {
        npc->status.equippedSpells.push_back(spellId);
        LOGI("Spell %u equipped to NPC %u", spellId, npcId);
    }
}

bool SpellManager::castSpell(uint32_t casterId, uint32_t spellId, uint32_t targetId) {
    if (!npcManager) return false;

    auto caster = npcManager->getNPC(casterId);
    auto target = npcManager->getNPC(targetId);

    if (!caster || !target) {
        LOGW("Caster or target not found");
        return false;
    }

    auto spell = getSpell(spellId);
    if (!spell) {
        LOGW("Spell ID %u not found", spellId);
        return false;
    }

    // マナチェック
    if (!spell->isAvailable(caster->status.currentMana)) {
        LOGW("NPC %u lacks mana to cast spell %u", casterId, spellId);
        return false;
    }

    // マナ消費
    if (!consumeMana(casterId, spell->manaCost)) {
        LOGW("Failed to consume mana for spell %u", spellId);
        return false;
    }

    // スペル効果適用
    applySpellEffect(target, *spell, caster->status);

    LOGI("Spell cast: %s (%s) cast %s(%s) on %s",
         caster->name.c_str(), spell->getSchoolNameJa().c_str(),
         spell->name.c_str(), spell->nameJa.c_str(),
         target->name.c_str());

    return true;
}

float SpellManager::calculateSpellDamage(const Spell& spell,
                                         const CharacterStatus& caster,
                                         const CharacterStatus& defender) {
    // ベースダメージ
    float baseDamage = spell.baseDamage;

    // 知性ボーナス（魔法系統の力）
    float intelligenceBonus = caster.getAttributeBonus("Intelligence") * 1.5f;

    // 意志力ボーナス（マナコントロール）
    float willpowerBonus = caster.getAttributeBonus("Willpower") * 0.5f;

    // 防御側の耐性計算
    float defenseReduction = defender.armorRating * 0.3f;

    // 最終ダメージ計算
    float totalDamage = baseDamage + intelligenceBonus + willpowerBonus - defenseReduction;

    if (totalDamage < 1.0f) {
        totalDamage = 1.0f;
    }

    return totalDamage;
}

bool SpellManager::consumeMana(uint32_t casterId, float amount) {
    if (!npcManager) return false;

    auto npc = npcManager->getNPC(casterId);
    if (!npc) return false;

    // Apply REDUCED_SPELL_COST cheat: 50% mana cost
    if (cheatManager && cheatManager->isCheatActive(CheatManager::CheatType::REDUCED_SPELL_COST)) {
        amount *= 0.5f;  // Half cost
    }

    // Apply NO_MAGICKA_DRAIN cheat: free spells
    if (cheatManager && cheatManager->isCheatActive(CheatManager::CheatType::NO_MAGICKA_DRAIN)) {
        amount = 0.0f;  // No mana cost
    }

    if (npc->status.currentMana < amount) {
        return false;
    }

    npc->status.currentMana -= amount;
    LOGD("NPC %u consumed %.1f mana (remaining: %.1f/%.1f)",
         casterId, amount, npc->status.currentMana, npc->status.maxMana);
    return true;
}

void SpellManager::applySpellEffect(std::shared_ptr<NPC> target, const Spell& spell,
                                   const CharacterStatus& caster) {
    if (!target) return;

    for (const auto& effect : spell.effects) {
        switch (effect.type) {
            case SpellEffectType::DAMAGE: {
                float damage = calculateSpellDamage(spell, caster, target->status);
                target->takeDamage(damage);
                LOGI("Damage applied: %.1f HP", damage);
                break;
            }

            case SpellEffectType::HEAL: {
                target->heal(effect.magnitude);
                LOGI("Healing applied: +%.1f HP", effect.magnitude);
                break;
            }

            case SpellEffectType::RESTORE_MANA: {
                target->status.currentMana += effect.magnitude;
                if (target->status.currentMana > target->status.maxMana) {
                    target->status.currentMana = target->status.maxMana;
                }
                LOGI("Mana restored: +%.1f", effect.magnitude);
                break;
            }

            case SpellEffectType::RESTORE_STAMINA: {
                target->status.stamina += effect.magnitude;
                if (target->status.stamina > target->status.maxStamina) {
                    target->status.stamina = target->status.maxStamina;
                }
                LOGI("Stamina restored: +%.1f", effect.magnitude);
                break;
            }

            case SpellEffectType::PARALYZE: {
                // TODO: 麻痺状態の実装
                LOGI("Paralyze effect applied to %s", target->name.c_str());
                break;
            }

            case SpellEffectType::INVISIBILITY: {
                // TODO: 透明化状態の実装
                LOGI("Invisibility effect applied to %s", target->name.c_str());
                break;
            }

            case SpellEffectType::FORTIFY_ATTR: {
                // TODO: 属性強化の実装
                LOGI("Fortify effect applied to %s", target->name.c_str());
                break;
            }

            case SpellEffectType::SUMMON: {
                // TODO: 召喚スペルの実装
                LOGI("Summon effect triggered");
                break;
            }

            default:
                break;
        }
    }
}

std::vector<std::shared_ptr<Spell>> SpellManager::getNpcSpells(uint32_t npcId) const {
    if (!npcManager) return {};

    auto npc = npcManager->getNPC(npcId);
    if (!npc) return {};

    std::vector<std::shared_ptr<Spell>> result;
    for (uint32_t spellId : npc->status.knownSpells) {
        auto spell = getSpell(spellId);
        if (spell) {
            result.push_back(spell);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Spell>> SpellManager::getNpcEquippedSpells(uint32_t npcId) const {
    if (!npcManager) return {};

    auto npc = npcManager->getNPC(npcId);
    if (!npc) return {};

    std::vector<std::shared_ptr<Spell>> result;
    for (uint32_t spellId : npc->status.equippedSpells) {
        auto spell = getSpell(spellId);
        if (spell) {
            result.push_back(spell);
        }
    }
    return result;
}

bool SpellManager::hasSpell(uint32_t npcId, uint32_t spellId) const {
    if (!npcManager) return false;

    auto npc = npcManager->getNPC(npcId);
    if (!npc) return false;

    auto it = std::find(npc->status.knownSpells.begin(),
                       npc->status.knownSpells.end(), spellId);
    return it != npc->status.knownSpells.end();
}

void SpellManager::logSpellStatus() const {
    LOGD("========== Spell Manager Status ==========");
    LOGD("Total spells: %zu", spells.size());

    for (const auto& pair : spells) {
        const auto& spell = pair.second;
        LOGD("  Spell: %s(%s) [%s]",
             spell->name.c_str(), spell->nameJa.c_str(),
             spell->getSchoolNameJa().c_str());
        LOGD("    ManaCost: %.1f, Damage: %.1f, Effects: %zu",
             spell->manaCost, spell->baseDamage, spell->effects.size());
    }

    LOGD("==========================================");
}
