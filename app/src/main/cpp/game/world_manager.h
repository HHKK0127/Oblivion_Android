#pragma once

#include "npc_manager.h"
#include <memory>
#include <android/log.h>

#define LOG_TAG "WorldManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class WorldManager {
private:
    std::shared_ptr<NpcManager> npcManager;

public:
    WorldManager();
    ~WorldManager();

    bool initialize();
    void cleanup();
    void update(float deltaTime);

    NpcManager* getNpcManager() const { return npcManager.get(); }
};
