#pragma once

#include "npc.h"
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
    uint32_t nextNpcId;

public:
    NpcManager();
    ~NpcManager();

    bool initialize();
    void cleanup();
    void update(float deltaTime);

    // NPC Management
    std::shared_ptr<NPC> createNPC(const std::string& name, const glm::vec3& position);
    std::shared_ptr<NPC> getNPC(uint32_t npcId) const;
    void removeNPC(uint32_t npcId);

    // Query
    std::vector<std::shared_ptr<NPC>> getAllNPCs() const;
    std::vector<std::shared_ptr<NPC>> getNPCsInArea(const glm::vec3& center, float radius) const;

    size_t getNPCCount() const { return npcs.size(); }

    void logNpcStatus() const;
};
