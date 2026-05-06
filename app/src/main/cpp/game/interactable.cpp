#include "interactable.h"
#include "cell.h"
#include <glm/gtc/type_ptr.hpp>
#include <android/log.h>
#include <cmath>

#define LOG_TAG "Interactable"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Interactable::Interactable(ObjectType type, std::shared_ptr<WorldObject> worldObj)
    : type(type), state(InteractionState::IDLE), worldObject(worldObj),
      interactionRadius(2.0f), enabled(true), highlightIntensity(0.0f),
      highlightTimer(0.0f) {

    if (!worldObject) {
        LOGE("Interactable created with null WorldObject");
    }
}

bool Interactable::isInRange(const glm::vec3& playerPos, float interactionRadius) {
    if (!worldObject) return false;

    // Calculate distance squared to avoid expensive sqrt operation
    glm::vec3 diff = playerPos - worldObject->position;
    float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    float radiusSq = interactionRadius * interactionRadius;
    return distanceSq <= radiusSq;
}

void Interactable::onPlayerEnterRange() {
    if (state != InteractionState::HIGHLIGHTED) {
        uint32_t objId = worldObject ? worldObject->objectId : 0;
        LOGD("Player entered range of interactable (objectId: %u)", objId);
        setInteractionState(InteractionState::HIGHLIGHTED);
    }
}

void Interactable::onPlayerExitRange() {
    if (state == InteractionState::HIGHLIGHTED) {
        uint32_t objId = worldObject ? worldObject->objectId : 0;
        LOGD("Player exited range of interactable (objectId: %u)", objId);
        setInteractionState(InteractionState::IDLE);
    }
}

bool Interactable::onInteract(const glm::vec3& /* playerPos */) {
    if (!enabled) return false;

    uint32_t objId = worldObject ? worldObject->objectId : 0;
    LOGD("Interactable interaction triggered (objectId: %u)", objId);
    setInteractionState(InteractionState::INTERACTING);
    return true;
}

void Interactable::update(float deltaTime) {
    if (!enabled) return;

    // Update highlight animation timer
    highlightTimer += deltaTime;

    // Pulsing highlight effect
    if (state == InteractionState::HIGHLIGHTED) {
        highlightIntensity = 0.5f + 0.5f * sinf(highlightTimer * 3.0f);
    } else {
        highlightIntensity = 0.0f;
    }
}

void Interactable::setInteractionState(InteractionState newState) {
    if (state != newState) {
        state = newState;
        highlightTimer = 0.0f;
    }
}
