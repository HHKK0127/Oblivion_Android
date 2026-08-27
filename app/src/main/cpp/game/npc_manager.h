#pragma once

#include "npc.h"
#include "spell.h"
#include "../assets/esm_reader.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <android/log.h>

#define LOG_TAG "NpcManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class NpcManager {
private:
    std::unordered_map<uint32_t, std::shared_ptr<NPC>> npcs;
    std::unordered_map<uint32_t, std::vector<uint32_t>> cellNpcs;  // cellId → NPC IDs
    std::unordered_map<uint32_t, uint32_t> npcToCell;              // npcId → cellId
    const oblivion::ESMManager* m_esm = nullptr;
    uint32_t nextNpcId;

    struct ActiveStatusEffect {
        SpellEffectType type;
        float remaining;
        float magnitude;
    };
    std::unordered_map<uint32_t, std::vector<ActiveStatusEffect>> m_statusEffects;

public:
    NpcManager();
    ~NpcManager();

    bool initialize();
    void cleanup();
    void update(float deltaTime);

    // NPC Management
    std::shared_ptr<NPC> createNPC(const std::string& name, const glm::vec3& position);
    std::shared_ptr<NPC> createNPCFromESM(uint32_t formID, const glm::vec3& position);
    std::shared_ptr<NPC> spawnFromLeveledList(uint32_t leveledListFormID,
                                               uint32_t playerLevel,
                                               const glm::vec3& position,
                                               int recursionDepth = 0);
    void setESMManager(const oblivion::ESMManager* esm) { m_esm = esm; }
    std::shared_ptr<NPC> getNPC(uint32_t npcId) const;
    void removeNPC(uint32_t npcId);

    // ESM-driven player initialization
    void initializePlayerFromESM(NPC& player, uint32_t raceFormID,
                                  uint32_t classFormID, uint32_t birthsignFormID);

    // Status effect tracking (BSGN/LVLI/quest-driven)
    void addStatusEffect(NPC& npc, SpellEffectType type, float duration, float magnitude);
    void updateStatusEffects(NPC& npc, float deltaTime);
    bool hasStatusEffect(const NPC& npc, SpellEffectType type) const;


    // Query
    std::vector<std::shared_ptr<NPC>> getAllNPCs() const;
    std::vector<std::shared_ptr<NPC>> getNPCsInArea(const glm::vec3& center, float radius) const;

    // Cell Integration (NEW)
    std::vector<std::shared_ptr<NPC>> getNpcsForCell(uint32_t cellId) const;
    void registerNpcToCell(uint32_t npcId, uint32_t cellId);
    void unregisterNpcFromCell(uint32_t npcId);
    uint32_t getNpcCell(uint32_t npcId) const;

    size_t getNPCCount() const { return npcs.size(); }

    void logNpcStatus() const;
};
