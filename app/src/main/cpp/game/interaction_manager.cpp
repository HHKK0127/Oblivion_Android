#include "interaction_manager.h"
#include "../world/world_manager.h"
#include "../world/world_data.h"
#include "../world/cell.h"
#include <algorithm>
#include <android/log.h>
#include <cmath>

#define LOG_TAG "InteractionManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

InteractionManager::InteractionManager()
    : worldManager(nullptr),
      interactionDistance(5.0f),
      lastPlayerPos(0.0f, 0.0f, 0.0f),
      proximityCheckTimer(0.0f) {
    LOGD("InteractionManager created");
}

InteractionManager::~InteractionManager() {
    clearInteractables();
    LOGD("InteractionManager destroyed");
}

bool InteractionManager::initialize(WorldManager* worldMgr) {
    if (!worldMgr) {
        LOGE("Cannot initialize InteractionManager with null WorldManager");
        return false;
    }

    worldManager = worldMgr;
    LOGI("InteractionManager initialized");
    return true;
}

std::shared_ptr<Door> InteractionManager::createDoor(std::shared_ptr<WorldObject> worldObj) {
    if (!worldObj) {
        LOGE("Cannot create door with null WorldObject");
        return nullptr;
    }

    auto door = std::make_shared<Door>(worldObj);
    registerInteractable(door);
    LOGD("Door created and registered (objectId: %u)", worldObj->objectId);
    return door;
}

std::shared_ptr<Container> InteractionManager::createContainer(std::shared_ptr<WorldObject> worldObj) {
    if (!worldObj) {
        LOGE("Cannot create container with null WorldObject");
        return nullptr;
    }

    auto container = std::make_shared<Container>(worldObj);
    registerInteractable(container);
    LOGD("Container created and registered (objectId: %u)", worldObj->objectId);
    return container;
}

void InteractionManager::registerInteractable(std::shared_ptr<Interactable> interactable) {
    if (!interactable || !interactable->getWorldObject()) {
        LOGE("Cannot register null interactable");
        return;
    }

    uint32_t objectId = interactable->getWorldObject()->objectId;
    interactables[objectId] = interactable;
    LOGD("Interactable registered (objectId: %u, total: %zu)", objectId, interactables.size());
}

void InteractionManager::unregisterInteractable(uint32_t refId) {
    auto it = interactables.find(refId);
    if (it != interactables.end()) {
        interactables.erase(it);
        LOGD("Interactable unregistered (refId: %u, total: %zu)", refId, interactables.size());
    }
}

void InteractionManager::update(const glm::vec3& playerPos, float deltaTime) {
    proximityCheckTimer += deltaTime;

    // Update all interactables
    for (auto& pair : interactables) {
        if (pair.second && pair.second->isEnabled()) {
            pair.second->update(deltaTime);
        }
    }

    // Update proximity detection periodically
    if (proximityCheckTimer >= PROXIMITY_UPDATE_INTERVAL) {
        proximityCheckTimer = 0.0f;
        updateProximity(playerPos);
        updateHighlightedObject(playerPos);
    }
}

void InteractionManager::updateProximity(const glm::vec3& playerPos) {
    nearbyInteractables.clear();
    nearbyInteractables.reserve(20);  // Pre-allocate space for typical nearby object count

    for (auto& pair : interactables) {
        if (!pair.second || !pair.second->isEnabled()) continue;

        if (pair.second->isInRange(playerPos, interactionDistance)) {
            nearbyInteractables.push_back(pair.second);
            pair.second->onPlayerEnterRange();
        } else {
            pair.second->onPlayerExitRange();
        }
    }
}

void InteractionManager::updateHighlightedObject(const glm::vec3& playerPos) {
    highlightedInteractable = findClosestInteractable(playerPos);
}

std::shared_ptr<Interactable> InteractionManager::findClosestInteractable(const glm::vec3& playerPos) {
    std::shared_ptr<Interactable> closest = nullptr;
    float closestDistanceSq = interactionDistance * interactionDistance;

    for (const auto& interactable : nearbyInteractables) {
        if (!interactable || !interactable->isEnabled()) continue;

        auto worldObj = interactable->getWorldObject();
        if (!worldObj) continue;

        // Calculate squared distance to avoid expensive sqrt operation
        glm::vec3 diff = playerPos - worldObj->position;
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distanceSq < closestDistanceSq) {
            closestDistanceSq = distanceSq;
            closest = interactable;
        }
    }

    return closest;
}

bool InteractionManager::interact(uint32_t interactableId, const glm::vec3& playerPos) {
    auto it = interactables.find(interactableId);
    if (it == interactables.end() || !it->second) {
        LOGW("Attempted interaction with non-existent interactable (id: %u)", interactableId);
        return false;
    }

    bool result = it->second->onInteract(playerPos);
    if (result) {
        LOGD("Interaction successful (id: %u)", interactableId);
    }
    return result;
}

std::shared_ptr<Interactable> InteractionManager::getInteractable(uint32_t refId) {
    auto it = interactables.find(refId);
    if (it != interactables.end()) {
        return it->second;
    }
    return nullptr;
}

void InteractionManager::clearInteractables() {
    interactables.clear();
    nearbyInteractables.clear();
    highlightedInteractable = nullptr;
    LOGI("InteractionManager cleared (%zu interactables removed)", interactables.size());
}
