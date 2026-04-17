#include "world_manager.h"

WorldManager::WorldManager() {
    LOGD("WorldManager created");
    npcManager = std::make_shared<NpcManager>();
}

WorldManager::~WorldManager() {
    cleanup();
    LOGD("WorldManager destroyed");
}

bool WorldManager::initialize() {
    LOGI("WorldManager initializing");
    if (!npcManager || !npcManager->initialize()) {
        LOGE("Failed to initialize NpcManager");
        return false;
    }
    return true;
}

void WorldManager::cleanup() {
    if (npcManager) {
        npcManager->cleanup();
    }
}

void WorldManager::update(float deltaTime) {
    if (npcManager) {
        npcManager->update(deltaTime);
    }
}
