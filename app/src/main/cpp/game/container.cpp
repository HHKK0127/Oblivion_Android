#include "container.h"
#include "cell.h"
#include "../world/world_data.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG "Container"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
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
    LOGD("Container created (objectId: %u)", worldObject ? worldObject->objectId : 0);
}

void Container::addItem(const InventoryItem& item) {
    if (!canAddItem(item)) {
        LOGE("Cannot add item (capacity exceeded): itemId=%u, weight=%.2f",
             item.item.itemId, item.item.weight);
        return;
    }

    // Check if item already exists and stack it
    for (auto& existingItem : items) {
        if (existingItem.item.itemId == item.item.itemId && existingItem.item.type == item.item.type) {
            existingItem.quantity += item.quantity;
            totalWeight += item.item.weight * item.quantity;
            LOGD("Stacked item (id: %u, new quantity: %u)", item.item.itemId, existingItem.quantity);
            return;
        }
    }

    // Add new item
    items.push_back(item);
    totalWeight += item.item.weight * item.quantity;
    LOGD("Item added to container (id: %u, qty: %u, weight: %.2f)",
         item.item.itemId, item.quantity, item.item.weight);
}

bool Container::removeItem(uint32_t itemId, uint32_t quantity) {
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->item.itemId == itemId) {
            if (it->quantity <= quantity) {
                totalWeight -= it->item.weight * it->quantity;
                items.erase(it);
                LOGD("Item removed from container (id: %u)", itemId);
            } else {
                it->quantity -= quantity;
                totalWeight -= it->item.weight * quantity;
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
    float itemTotalWeight = item.item.weight * item.quantity;
    return (totalWeight + itemTotalWeight) <= capacity;
}

bool Container::onInteract(const glm::vec3& playerPos) {
    if (!enabled) return false;

    if (locked) {
        LOGD("Container is locked (objectId: %u)", worldObject->objectId);
        return false;
    }

    // Toggle container state
    if (containerState == ContainerState::CLOSED || containerState == ContainerState::CLOSING) {
        containerState = ContainerState::OPENING;
        animationProgress = 0.0f;
        LOGD("Container opening (objectId: %u, items: %zu)", worldObject->objectId, items.size());
    } else if (containerState == ContainerState::OPEN || containerState == ContainerState::OPENING) {
        containerState = ContainerState::CLOSING;
        animationProgress = 1.0f;
        LOGD("Container closing (objectId: %u)", worldObject->objectId);
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

    // Update world object scale based on animation progress (uniform scaling)
    if (worldObject) {
        float scaleMultiplier = 1.0f + (openScale - 1.0f) * animationProgress;
        worldObject->scale = scaleMultiplier;
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
        totalWeight += item.item.weight * item.quantity;
    }
}
