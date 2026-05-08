#pragma once

#include "interactable.h"
#include "item.h"
#include <vector>
#include <string>

// Use InventoryItem from item.h (alias for InventorySlot)

/**
 * Container - An interactable container that holds items
 * Supports chests, barrels, cupboards, etc.
 */
class Container : public Interactable {
public:
    enum class ContainerState {
        CLOSED,
        OPENING,
        OPEN,
        CLOSING
    };

    Container(std::shared_ptr<WorldObject> worldObj);
    ~Container() override = default;

    // Container state
    ContainerState getContainerState() const { return containerState; }
    bool isLocked() const { return locked; }
    void setLocked(bool lock) { locked = lock; }

    // Inventory management
    void addItem(const InventoryItem& item);
    bool removeItem(uint32_t itemId, uint32_t quantity = 1);
    const std::vector<InventoryItem>& getItems() const { return items; }
    size_t getItemCount() const { return items.size(); }
    float getTotalWeight() const { return totalWeight; }
    float getCapacity() const { return capacity; }

    // Capacity management
    bool canAddItem(const InventoryItem& item) const;
    void setCapacity(float cap) { capacity = cap; }

    // Interaction
    bool onInteract(const glm::vec3& playerPos) override;
    void update(float deltaTime) override;

    // Animation properties
    float getOpenScale() const { return openScale; }
    void setOpenScale(float scale) { openScale = scale; }
    float getAnimationDuration() const { return animationDuration; }
    void setAnimationDuration(float duration) { animationDuration = duration; }

    // Get current animation state (0.0 = closed, 1.0 = open)
    float getAnimationProgress() const { return animationProgress; }

private:
    ContainerState containerState;
    bool locked;
    std::vector<InventoryItem> items;
    float totalWeight;
    float capacity;  // Maximum weight capacity
    float openScale;  // Scale multiplier when open (0.0 = closed, 1.0 = open)
    float animationDuration;
    float animationProgress;  // 0.0 to 1.0

    void updateAnimation(float deltaTime);
    void updateInteractionText();
    void recalculateWeight();
};
