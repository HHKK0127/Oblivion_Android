#include "container.h"
#include "cell.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG "Container"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

Container::Container(std::shared_ptr<WorldObject> worldObj)
    : Interactable(ObjectType::CONTAINER, worldObj),
      containerState(ContainerState::CLOSED),
      locked(false),
      totalWeight(0.0f),
      capacity(100.0f),  // Default capacity 100 units
      openScale(1.0f),
      animationDuration(0.3f),  // 300ms animation
      animationProgress(0.0f) {

    if (worldObject) {
        interactionRadius = 1.0f;
    }
    updateInteractionText();
    LOGD("Container created (refId: %u)", worldObject ? worldObject->refId : 0);
}

void Container::addItem(const InventoryItem& item) {
    if (!canAddItem(item)) {
        LOGE("Cannot add item (capacity exceeded): itemId=%u, weight=%.2f",
             item.itemId, item.weight);
        return;
    }

    // Check if item already exists and stack it
    for (auto& existingItem : items) {
        if (existingItem.itemId == item.itemId && existingItem.itemType == item.itemType) {
            existingItem.quantity += item.quantity;
            totalWeight += item.weight * item.quantity;
            LOGD("Stacked item (id: %u, new quantity: %u)", item.itemId, existingItem.quantity);
            return;
        }
    }

    // Add new item
    items.push_back(item);
    totalWeight += item.weight * item.quantity;
    LOGD("Item added to container (id: %u, qty: %u, weight: %.2f)",
         item.itemId, item.quantity, item.weight);
}

bool Container::removeItem(uint32_t itemId, uint32_t quantity) {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->itemId == itemId) {
            if (it->quantity <= quantity) {
                totalWeight -= it->weight * it->quantity;
                items.erase(it);
                LOGD("Item removed from container (id: %u)", itemId);
            } else {
                it->quantity -= quantity;
                totalWeight -= it->weight * quantity;
                LOGD("Item quantity decreased (id: %u, new qty: %u)", itemId, it->quantity);
            }
            recalculateWeight();
            return true;
        }
    }
    LOGW("Item not found in container (id: %u)", itemId);
    return false;
}

bool Container::canAddItem(const InventoryItem& item) const {
    float itemTotalWeight = item.weight * item.quantity;
    return (totalWeight + itemTotalWeight) <= capacity;
}

bool Container::onInteract(const glm::vec3& playerPos) {
    if (!enabled) return false;

    if (locked) {
        LOGD("Container is locked (refId: %u)", worldObject->refId);
        return false;
    }

    // Toggle container state
    if (containerState == ContainerState::CLOSED || containerState == ContainerState::CLOSING) {
        containerState = ContainerState::OPENING;
        animationProgress = 0.0f;
        LOGD("Container opening (refId: %u, items: %zu)", worldObject->refId, items.size());
    } else if (containerState == ContainerState::OPEN || containerState == ContainerState::OPENING) {
        containerState = ContainerState::CLOSING;
        animationProgress = 1.0f;
        LOGD("Container closing (refId: %u)", worldObject->refId);
    }

    Interactable::onInteract(playerPos);
    updateInteractionText();
    return true;
}

void Container::update(float deltaTime) {
    Interactable::update(deltaTime);

    if (!enabled) return;

    // Update animation
    if (containerState == ContainerState::OPENING || containerState == ContainerState::CLOSING) {
        updateAnimation(deltaTime);
    }
}

void Container::updateAnimation(float deltaTime) {
    if (containerState == ContainerState::OPENING) {
        animationProgress += deltaTime / animationDuration;
        if (animationProgress >= 1.0f) {
            animationProgress = 1.0f;
            containerState = ContainerState::OPEN;
            LOGD("Container fully opened");
        }
    } else if (containerState == ContainerState::CLOSING) {
        animationProgress -= deltaTime / animationDuration;
        if (animationProgress <= 0.0f) {
            animationProgress = 0.0f;
            containerState = ContainerState::CLOSED;
            LOGD("Container fully closed");
        }
    }

    // Update world object scale based on animation progress
    if (worldObject) {
        float scaleMultiplier = 1.0f + (openScale - 1.0f) * animationProgress;
        worldObject->scale.y = scaleMultiplier;
    }
}

void Container::updateInteractionText() {
    if (locked) {
        interactionText = "Unlock Container";
    } else {
        switch (containerState) {
            case ContainerState::CLOSED:
            case ContainerState::CLOSING:
                interactionText = "Open Container";
                break;
            case ContainerState::OPEN:
            case ContainerState::OPENING:
                interactionText = "Close Container";
                break;
        }
    }
}

void Container::recalculateWeight() {
    totalWeight = 0.0f;
    for (const auto& item : items) {
        totalWeight += item.weight * item.quantity;
    }
}
