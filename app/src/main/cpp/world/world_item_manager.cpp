#include "world_item_manager.h"
#include <algorithm>

bool WorldItemManager::initialize(std::shared_ptr<inventory::InventoryGrid> inv,
                                  inventory::ItemFactory* itemFac) {
    if (!inv || !itemFac) {
        LOGE("WorldItemManager initialize failed: null pointers");
        return false;
    }
    inventory = inv;
    itemFactory = itemFac;
    LOGI("WorldItemManager initialized");
    return true;
}

uint32_t WorldItemManager::dropItem(uint32_t itemId, uint32_t quantity, const glm::vec3& position) {
    if (!itemFactory) {
        LOGE("WorldItemManager not initialized");
        return 0;
    }

    // Create item from factory
    inventory::Item item = itemFactory->createItem(itemId, quantity);
    if (item.id == 0) {
        LOGE("Item ID %u not found in factory", itemId);
        return 0;
    }

    // Create world item
    uint32_t worldItemId = generateWorldItemId();
    WorldItem worldItem(worldItemId, itemId, item.name, item.name, position);
    worldItems.push_back(worldItem);

    LOGI("Dropped item: %s (qty %u) at (%.1f, %.1f, %.1f)",
         item.name.c_str(), quantity, position.x, position.y, position.z);

    return worldItemId;
}

bool WorldItemManager::pickupItem(uint32_t worldItemId, const glm::vec3& playerPos, float pickupRange) {
    if (!inventory || !itemFactory) {
        LOGE("WorldItemManager not initialized");
        return false;
    }

    // Find world item
    auto it = std::find_if(worldItems.begin(), worldItems.end(),
        [worldItemId](const WorldItem& item) { return item.worldItemId == worldItemId; });

    if (it == worldItems.end()) {
        LOGD("World item %u not found", worldItemId);
        return false;
    }

    WorldItem& worldItem = *it;

    // Check if in pickup range
    if (!worldItem.isInPickupRange(playerPos, pickupRange)) {
        LOGD("World item %u out of pickup range (dist %.1f, range %.1f)",
             worldItemId, worldItem.getDistanceTo(playerPos), pickupRange);
        return false;
    }

    // Get item template
    inventory::Item item = itemFactory->createItem(worldItem.itemId, 1);
    if (item.id == 0) {
        LOGE("Item ID %u not found in factory", worldItem.itemId);
        return false;
    }

    // Try to add to inventory
    if (!inventory->addItem(item, 1)) {
        LOGE("Failed to add item %s to inventory (possibly full)", item.name.c_str());
        return false;
    }

    // Mark as picked up
    worldItem.isPickedUp = true;
    LOGI("Picked up: %s (world item %u)", item.name.c_str(), worldItemId);
    return true;
}

void WorldItemManager::checkItemsInRange(const glm::vec3& playerPos, float pickupRange, bool autoPickup) {
    std::vector<uint32_t> itemsToPickup;

    for (const auto& item : worldItems) {
        if (!item.isPickedUp && item.isInPickupRange(playerPos, pickupRange)) {
            itemsToPickup.push_back(item.worldItemId);
        }
    }

    if (autoPickup) {
        for (uint32_t worldItemId : itemsToPickup) {
            pickupItem(worldItemId, playerPos, pickupRange);
        }
    }
}

std::vector<uint32_t> WorldItemManager::getItemsInRange(const glm::vec3& playerPos, float range) const {
    std::vector<uint32_t> result;
    for (const auto& item : worldItems) {
        if (!item.isPickedUp && item.isInPickupRange(playerPos, range)) {
            result.push_back(item.worldItemId);
        }
    }
    return result;
}

void WorldItemManager::clearPickedUpItems() {
    auto newEnd = std::remove_if(worldItems.begin(), worldItems.end(),
        [](const WorldItem& item) { return item.isPickedUp; });

    size_t removed = std::distance(newEnd, worldItems.end());
    worldItems.erase(newEnd, worldItems.end());

    if (removed > 0) {
        LOGD("Cleared %zu picked-up items", removed);
    }
}

std::vector<std::pair<uint32_t, glm::vec3>> WorldItemManager::getVisibleItems(
    const glm::vec3& playerPos, float range) const {
    std::vector<std::pair<uint32_t, glm::vec3>> result;
    for (const auto& item : worldItems) {
        if (!item.isPickedUp && item.isInPickupRange(playerPos, range)) {
            result.push_back({item.worldItemId, item.position});
        }
    }
    return result;
}

uint32_t WorldItemManager::generateWorldItemId() {
    return nextWorldItemId++;
}
