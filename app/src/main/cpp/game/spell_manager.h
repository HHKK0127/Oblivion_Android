#pragma once

#include "spell.h"
#include "npc_manager.h"
#include "player.h"
#include "../assets/esm_reader.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <android/log.h>

#define LOG_TAG "SpellManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class SpellManager {
private:
    std::unordered_map<uint32_t, std::shared_ptr<Spell>> spells;  // spellId → Spell
    NpcManager* npcManager;
    class CheatManager* cheatManager;
    uint32_t nextSpellId;

public:
    SpellManager();
    ~SpellManager();

    bool initialize(NpcManager* nm, class CheatManager* cm = nullptr);
    void cleanup();
    void update(float deltaTime);

    // Spell creation and management
    uint32_t createSpell(const std::string& name, const std::string& nameJa,
                        MagicSchool school, float manaCost, float baseDamage);

    // Load spells from ESM data
    void loadSpellsFromESM(const oblivion::ESMManager& esmMgr);

    std::shared_ptr<Spell> getSpell(uint32_t spellId) const;
    void addEffectToSpell(uint32_t spellId, const SpellEffect& effect);

    // Assign spells to NPCs
    void teachSpellToNpc(uint32_t npcId, uint32_t spellId);
    void equipSpellToNpc(uint32_t npcId, uint32_t spellId);

    // Spell casting
    bool castSpell(uint32_t casterId, uint32_t spellId, uint32_t targetId);

    // Direct player cast (without NpcManager)
    bool castPlayerSpell(Player* player, uint32_t spellId, uint32_t targetId);

    // Damage calculation
    float calculateSpellDamage(const Spell& spell, const CharacterStatus& caster,
                              const CharacterStatus& defender);

    // Mana consumption
    bool consumeMana(uint32_t casterId, float amount);

    // Spell effect application
    void applySpellEffect(std::shared_ptr<NPC> target, const Spell& spell,
                         const CharacterStatus& caster);

    // Queries
    std::vector<std::shared_ptr<Spell>> getNpcSpells(uint32_t npcId) const;
    std::vector<std::shared_ptr<Spell>> getNpcEquippedSpells(uint32_t npcId) const;
    bool hasSpell(uint32_t npcId, uint32_t spellId) const;

    // Logging
    void logSpellStatus() const;
};
