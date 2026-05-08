#pragma once

#include "spell.h"
#include "npc_manager.h"
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

    // スペル作成と管理
    uint32_t createSpell(const std::string& name, const std::string& nameJa,
                        MagicSchool school, float manaCost, float baseDamage);
    std::shared_ptr<Spell> getSpell(uint32_t spellId) const;
    void addEffectToSpell(uint32_t spellId, const SpellEffect& effect);

    // NPCへのスペル割り当て
    void teachSpellToNpc(uint32_t npcId, uint32_t spellId);
    void equipSpellToNpc(uint32_t npcId, uint32_t spellId);

    // スペルキャスト
    bool castSpell(uint32_t casterId, uint32_t spellId, uint32_t targetId);

    // ダメージ計算
    float calculateSpellDamage(const Spell& spell, const CharacterStatus& caster,
                              const CharacterStatus& defender);

    // マナ消費
    bool consumeMana(uint32_t casterId, float amount);

    // スペル効果適用
    void applySpellEffect(std::shared_ptr<NPC> target, const Spell& spell,
                         const CharacterStatus& caster);

    // クエリ
    std::vector<std::shared_ptr<Spell>> getNpcSpells(uint32_t npcId) const;
    std::vector<std::shared_ptr<Spell>> getNpcEquippedSpells(uint32_t npcId) const;
    bool hasSpell(uint32_t npcId, uint32_t spellId) const;

    // ロギング
    void logSpellStatus() const;
};
