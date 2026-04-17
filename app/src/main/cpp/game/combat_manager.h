#pragma once

#include "npc.h"
#include "npc_manager.h"
#include "world_manager.h"
#include <unordered_map>
#include <memory>
#include <android/log.h>

#define LOG_TAG "CombatManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct CombatInstance {
    std::shared_ptr<NPC> attacker;
    std::shared_ptr<NPC> defender;
    float lastAttackTime;
    float combatDuration;
};

class CombatManager {
private:
    WorldManager* worldManager;
    NpcManager* npcManager;
    class SpellManager* spellManager;
    std::unordered_map<uint32_t, CombatInstance> activeCombats;

    static constexpr float DAMAGE_CALCULATION_COOLDOWN = 1.0f;

public:
    CombatManager();
    ~CombatManager();

    bool initialize(WorldManager* wm, NpcManager* nm, class SpellManager* sm = nullptr);
    void cleanup();
    void update(float deltaTime);

    void initiateCombat(std::shared_ptr<NPC> attacker, std::shared_ptr<NPC> defender);
    void endCombat(uint32_t defenderId);

    float calculateDamage(const CharacterStatus& attacker, const CharacterStatus& defender);
    void applyDamage(std::shared_ptr<NPC> target, float damage);
    void applyHeal(std::shared_ptr<NPC> target, float amount);

    std::shared_ptr<NPC> findNearestEnemy(std::shared_ptr<NPC> npc, float detectionRadius = 30.0f);
    bool isInCombat(uint32_t npcId) const;
    const CombatInstance* getCombat(uint32_t defenderId) const;

    void clearCombats();
    void logCombatStatus() const;
};
