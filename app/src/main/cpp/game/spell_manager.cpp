#include "spell_manager.h"
#include "../audio/audio_manager.h"
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers for mapping ESM spell data to runtime types
// ---------------------------------------------------------------------------

/// Map Oblivion magic school byte (SPIT byte 12) to our MagicSchool enum
static MagicSchool esmSchoolToEnum(uint32_t schoolByte) {
    switch (schoolByte) {
        case 0:  return MagicSchool::ALTERATION;
        case 1:  return MagicSchool::CONJURATION;
        case 2:  return MagicSchool::DESTRUCTION;
        case 3:  return MagicSchool::ILLUSION;
        case 4:  return MagicSchool::MYSTICISM;
        case 5:  return MagicSchool::RESTORATION;
        default: return MagicSchool::DESTRUCTION;
    }
}

/// Map a MGEF effect FormID to our SpellEffectType.
/// Known Oblivion MGEF FormIDs (from Oblivion.esm):
///   0x0001E825 = FireDamage   (DAMAGE)
///   0x0001E831 = Heal          (HEAL)
///   0x0001E84F = RestoreMagicka (RESTORE_MANA)
///   0x0001E82E = RestoreStamina (RESTORE_STAMINA)
///   0x00027FAC = Paralyze      (PARALYZE)
///   0x0001E851 = Invisibility  (INVISIBILITY)
///   0x0001E843 = FortifyAttribute (FORTIFY_ATTR)
///   0x0001E848 = Summon       (SUMMON)
///   0x0001E835 = Shield       (DAMAGE — treated as buff)
static SpellEffectType mgefToEffectType(uint32_t mgefFormID) {
    switch (mgefFormID) {
        case 0x0001E825: return SpellEffectType::DAMAGE;         // FireDamage
        case 0x0001E826: return SpellEffectType::DAMAGE;         // FrostDamage
        case 0x0001E827: return SpellEffectType::DAMAGE;         // ShockDamage
        case 0x0001E831: return SpellEffectType::HEAL;           // Heal
        case 0x0001E84F: return SpellEffectType::RESTORE_MANA;   // RestoreMagicka
        case 0x0001E82E: return SpellEffectType::RESTORE_STAMINA; // RestoreStamina
        case 0x00027FAC: return SpellEffectType::PARALYZE;       // Paralyze
        case 0x0001E851: return SpellEffectType::INVISIBILITY;   // Invisibility
        case 0x0001E843: return SpellEffectType::FORTIFY_ATTR;   // FortifyAttribute
        case 0x0001E848: return SpellEffectType::SUMMON;         // Summon (generic)
        default:         return SpellEffectType::DAMAGE;         // fallback
    }
}

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

void SpellManager::loadSpellsFromESM(const oblivion::ESMManager& esmMgr) {
    const auto& esmSpells = esmMgr.getAllSpells();
    size_t loaded = 0;

    for (const auto& s : esmSpells) {
        if (s.formID == 0) continue;
        if (spells.count(s.formID) != 0) continue;

        // Use ESM FormID directly as spell ID (so NPC spell lists stay valid)
        auto spell = std::make_shared<Spell>(
            s.formID,
            s.fullName.empty() ? s.editorID : s.fullName,
            s.fullName,
            esmSchoolToEnum(s.effectType),
            static_cast<float>(s.cost),
            0.0f  // baseDamage set from first DAMAGE effect
        );

        // Convert EFID/EFIT pairs to SpellEffect list
        size_t effectCount = std::min({
            s.effectFormIDs.size(),
            s.effectMagnitudes.size(),
            s.effectAreas.size(),
            s.effectDurations.size()
        });
        for (size_t ei = 0; ei < effectCount; ++ei) {
            SpellEffect effect(
                mgefToEffectType(s.effectFormIDs[ei]),
                s.effectMagnitudes[ei],
                static_cast<float>(s.effectDurations[ei])
            );
            spell->effects.push_back(effect);

            // Use first DAMAGE effect magnitude as base damage
            if (effect.type == SpellEffectType::DAMAGE && spell->baseDamage <= 0.0f) {
                spell->baseDamage = s.effectMagnitudes[ei];
            }
        }

        spells[s.formID] = spell;
        ++loaded;

        if (loaded <= 10) {
            LOGI("  Spell[%zu]: 0x%08X '%s' school=%u cost=%u effects=%zu",
                 loaded, s.formID, spell->name.c_str(),
                 static_cast<unsigned>(s.effectType), s.cost, effectCount);
        }
    }

    LOGI("SpellManager: Loaded %zu spells from ESM data", loaded);
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

    // Play spell cast sound based on school
    if (g_audioManager) {
        std::string soundKey;
        switch (spell->school) {
            case MagicSchool::DESTRUCTION: soundKey = "magic/destruction_cast"; break;
            case MagicSchool::RESTORATION: soundKey = "magic/restoration_cast"; break;
            case MagicSchool::CONJURATION: soundKey = "magic/conjuration_cast"; break;
            case MagicSchool::ALTERATION:  soundKey = "magic/alteration_cast"; break;
            case MagicSchool::ILLUSION:    soundKey = "magic/illusion_cast"; break;
            case MagicSchool::MYSTICISM:   soundKey = "magic/mysticism_cast"; break;
            default: break;
        }
        if (!soundKey.empty()) {
            g_audioManager->playSound(soundKey);
        }
    }

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
    // Cheat manager not available - disabled
    // if (cheatManager && cheatManager->isCheatActive(CheatManager::CheatType::REDUCED_SPELL_COST)) {
    //     amount *= 0.5f;  // Half cost
    // }

    // Apply NO_MAGICKA_DRAIN cheat: free spells
    // Cheat manager not available - disabled
    // if (cheatManager && cheatManager->isCheatActive(CheatManager::CheatType::NO_MAGICKA_DRAIN)) {
    //     amount = 0.0f;  // No mana cost
    // }

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
                // Paralyze: immobilize target for effect duration
                // In Oblivion, paralyze prevents all movement and actions
                float duration = (effect.duration > 0.0f) ? effect.duration : 3.0f;
                if (npcManager) {
                    npcManager->addStatusEffect(*target, SpellEffectType::PARALYZE,
                                                duration, effect.magnitude);
                }
                // Force target out of combat movement
                target->setAIState(AIState::IDLE);
                LOGI("Paralyze: %s paralyzed for %.1fs (magnitude=%.0f)",
                     target->name.c_str(), duration, effect.magnitude);
                break;
            }

            case SpellEffectType::INVISIBILITY: {
                // Invisibility: target becomes undetectable for duration
                // In Oblivion, invisibility breaks on attack/spell cast
                float duration = (effect.duration > 0.0f) ? effect.duration : 15.0f;
                if (npcManager) {
                    npcManager->addStatusEffect(*target, SpellEffectType::INVISIBILITY,
                                                duration, effect.magnitude);
                }
                // Exit combat when becoming invisible
                if (target->inCombat) {
                    target->exitCombat();
                }
                LOGI("Invisibility: %s invisible for %.1fs (magnitude=%.0f)",
                     target->name.c_str(), duration, effect.magnitude);
                break;
            }

            case SpellEffectType::FORTIFY_ATTR: {
                // Fortify Attribute: temporarily boost a target attribute
                // In Oblivion, this increases Strength/Intelligence/etc by magnitude
                float duration = (effect.duration > 0.0f) ? effect.duration : 60.0f;
                if (npcManager) {
                    npcManager->addStatusEffect(*target, SpellEffectType::FORTIFY_ATTR,
                                                duration, effect.magnitude);
                }
                // Apply immediate attribute boost
                // Default to Strength if no specific attribute is set
                std::string attr = effect.affectedAttribute.empty()
                                   ? "Strength" : effect.affectedAttribute;
                auto attrIt = target->status.attributes.find(attr);
                if (attrIt != target->status.attributes.end()) {
                    attrIt->second += effect.magnitude;
                    LOGI("Fortify: %s %s boosted by %.0f (now %.0f) for %.1fs",
                         target->name.c_str(), attr.c_str(), effect.magnitude,
                         attrIt->second, duration);
                } else {
                    // Attribute not found, apply to Strength as fallback
                    target->status.attributes["Strength"] += effect.magnitude;
                    LOGI("Fortify: %s Strength boosted by %.0f for %.1fs (attr '%s' not found)",
                         target->name.c_str(), effect.magnitude, duration, attr.c_str());
                }
                break;
            }

            case SpellEffectType::SUMMON: {
                // Summon: create a temporary allied creature
                // In Oblivion, summons last for the spell duration and fight for the caster
                float duration = (effect.duration > 0.0f) ? effect.duration : 60.0f;
                if (npcManager) {
                    npcManager->addStatusEffect(*target, SpellEffectType::SUMMON,
                                                duration, effect.magnitude);
                }
                // Spawn a summoned creature near the caster's position
                // Use magnitude as the creature level hint
                glm::vec3 spawnPos = target->position + glm::vec3(2.0f, 0.0f, 2.0f);
                auto summoned = npcManager->createNPC("Summoned Creature", spawnPos);
                if (summoned) {
                    summoned->status.maxHealth = effect.magnitude * 5.0f;
                    summoned->status.currentHealth = summoned->status.maxHealth;
                    summoned->status.weaponDamage = effect.magnitude;
                    summoned->moveSpeed = 6.0f;
                    LOGI("Summon: creature spawned at (%.1f, %.1f, %.1f) HP=%.0f DMG=%.0f for %.1fs",
                         spawnPos.x, spawnPos.y, spawnPos.z,
                         summoned->status.maxHealth, effect.magnitude, duration);
                }
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
