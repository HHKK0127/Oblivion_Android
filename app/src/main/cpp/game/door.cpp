#include "door.h"
#include "cell.h"
#include <glm/gtc/matrix_transform.hpp>
#include <android/log.h>
#include <cmath>

#define LOG_TAG "Door"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Door::Door(std::shared_ptr<WorldObject> worldObj)
    : Interactable(ObjectType::DOOR, worldObj),
      doorState(DoorState::CLOSED),
      locked(false),
      openAngle(90.0f),  // Default 90 degree rotation
      animationDuration(0.5f),  // 500ms animation
      animationProgress(0.0f),
      originalRotation(0.0f, 0.0f, 0.0f),
      currentRotation(0.0f, 0.0f, 0.0f) {

    if (worldObject) {
        originalRotation = worldObject->rotation;
        currentRotation = originalRotation;
        interactionRadius = 1.5f;
    }
    updateInteractionText();
    LOGD("Door created (refId: %u)", worldObject ? worldObject->refId : 0);
}

bool Door::onInteract(const glm::vec3& playerPos) {
    if (!enabled || locked) {
        LOGD("Door interaction blocked (locked: %s, enabled: %s)",
             locked ? "true" : "false", enabled ? "true" : "false");
        return false;
    }

    // Toggle door state
    if (doorState == DoorState::CLOSED || doorState == DoorState::CLOSING) {
        doorState = DoorState::OPENING;
        animationProgress = 0.0f;
        LOGD("Door opening (refId: %u)", worldObject->refId);
    } else if (doorState == DoorState::OPEN || doorState == DoorState::OPENING) {
        doorState = DoorState::CLOSING;
        animationProgress = 1.0f;
        LOGD("Door closing (refId: %u)", worldObject->refId);
    }

    Interactable::onInteract(playerPos);
    updateInteractionText();
    return true;
}

void Door::update(float deltaTime) {
    Interactable::update(deltaTime);

    if (!enabled) return;

    // Update animation
    if (doorState == DoorState::OPENING || doorState == DoorState::CLOSING) {
        updateAnimation(deltaTime);
    }
}

void Door::updateAnimation(float deltaTime) {
    if (doorState == DoorState::OPENING) {
        animationProgress += deltaTime / animationDuration;
        if (animationProgress >= 1.0f) {
            animationProgress = 1.0f;
            doorState = DoorState::OPEN;
            LOGD("Door fully opened");
        }
    } else if (doorState == DoorState::CLOSING) {
        animationProgress -= deltaTime / animationDuration;
        if (animationProgress <= 0.0f) {
            animationProgress = 0.0f;
            doorState = DoorState::CLOSED;
            LOGD("Door fully closed");
        }
    }

    // Update rotation based on animation progress
    // Smooth interpolation using ease-in-out
    float easeProgress = animationProgress;
    if (easeProgress < 0.5f) {
        easeProgress = 2.0f * easeProgress * easeProgress;
    } else {
        easeProgress = -1.0f + (4.0f - 2.0f * easeProgress) * easeProgress;
    }

    // Apply rotation around Y axis (common for doors)
    float targetRotationY = originalRotation.y + (openAngle * easeProgress);
    currentRotation.y = targetRotationY;

    if (worldObject) {
        worldObject->rotation = currentRotation;
    }
}

void Door::updateInteractionText() {
    if (locked) {
        interactionText = "Unlock Door";
    } else {
        switch (doorState) {
            case DoorState::CLOSED:
            case DoorState::CLOSING:
                interactionText = "Open Door";
                break;
            case DoorState::OPEN:
            case DoorState::OPENING:
                interactionText = "Close Door";
                break;
        }
    }
}
