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

void Container::populateFromESM(const oblivion::ESMManager& esm, uint32_t containerFormID) {
    const oblivion::ContainerData* contData = esm.findContainer(containerFormID);
    if (!contData) {
        LOGW("populateFromESM: Container 0x%08X not found", containerFormID);
        return;
    }

    // Set capacity from ESM data
    if (contData->weight > 0.0f) {
        capacity = contData->weight;
    }

    // Populate items from ESM CNTO entries
    for (const auto& entry : contData->items) {
        if (entry.itemFormID == 0) continue;

        // Resolve item name and properties from various ESM record types
        Item item;
        item.itemId = entry.itemFormID;
        item.stackSize = entry.count;

        // Try to resolve from different record types
        const oblivion::WeaponData* weap = esm.findWeapon(entry.itemFormID);
        if (weap) {
            item.name = weap->fullName.empty() ? weap->editorID : weap->fullName;
            item.type = ItemType::WEAPON;
            item.weight = static_cast<float>(weap->weight);
            item.value = weap->value;
        } else {
            const oblivion::ArmorData* armo = esm.findArmor(entry.itemFormID);
            if (armo) {
                item.name = armo->fullName.empty() ? armo->editorID : armo->fullName;
                item.type = ItemType::ARMOR;
                item.weight = static_cast<float>(armo->weight);
                item.value = armo->value;
            } else {
                const oblivion::AlchemyData* alch = esm.findAlchemy(entry.itemFormID);
                if (alch) {
                    item.name = alch->fullName.empty() ? alch->editorID : alch->fullName;
                    item.type = ItemType::POTION;
                    item.weight = alch->weight;
                    item.value = alch->value;
                } else {
                    const oblivion::IngredientData* ingr = esm.findIngredient(entry.itemFormID);
                    if (ingr) {
                        item.name = ingr->fullName.empty() ? ingr->editorID : ingr->fullName;
                        item.type = ItemType::INGREDIENT;
                        item.weight = ingr->weight;
                        item.value = ingr->value;
                    } else {
                        const oblivion::MiscItemData* misc = esm.findMiscItem(entry.itemFormID);
                        if (misc) {
                            item.name = misc->fullName.empty() ? misc->editorID : misc->fullName;
                            item.type = ItemType::MISC;
                            item.weight = misc->weight;
                            item.value = misc->value;
                        } else {
                            const oblivion::BookData* book = esm.findBook(entry.itemFormID);
                            if (book) {
                                item.name = book->fullName.empty() ? book->editorID : book->fullName;
                                item.type = ItemType::BOOK;
                                item.weight = book->weight;
                                item.value = book->value;
                            } else {
                                // Unknown item — use formID as name
                                char buf[32];
                                snprintf(buf, sizeof(buf), "Item_%08X", entry.itemFormID);
                                item.name = buf;
                                item.type = ItemType::MISC;
                                item.weight = 0.1f;
                                item.value = 1;
                            }
                        }
                    }
                }
            }
        }

        InventoryItem invItem;
        invItem.item = item;
        invItem.quantity = entry.count;
        addItem(invItem);
    }

    LOGI("Container populated from ESM: formID=0x%08X, items=%zu, capacity=%.1f",
         containerFormID, items.size(), capacity);
}
