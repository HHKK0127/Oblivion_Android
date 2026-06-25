#pragma once

#include "world_item.h"
#include "../inventory/inventory_grid.h"
#include "../inventory/item_factory.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <android/log.h>

#define LOG_TAG "WorldItemManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * @brief ワールドアイテムマネージャー - ワールドのアイテム管理
 *
 * Phase 9B Week 4: ワールドに配置されたアイテムのドロップ・拾得管理
 * プレイヤーが近づいたら拾得可能になり、インベントリに追加
 */
class WorldItemManager {
public:
    WorldItemManager() = default;
    ~WorldItemManager() = default;

    // Initialize with inventory and item factory
    bool initialize(std::shared_ptr<inventory::InventoryGrid> inv,
                    inventory::ItemFactory* itemFactory);

    // Drop item to world at position
    // Returns world item ID if successful
    uint32_t dropItem(uint32_t itemId, uint32_t quantity, const glm::vec3& position);

    // Pick up item from world (if in range)
    // Returns true if successful
    bool pickupItem(uint32_t worldItemId, const glm::vec3& playerPos, float pickupRange);

    // Check items in pickup range and auto-pickup if enabled
    void checkItemsInRange(const glm::vec3& playerPos, float pickupRange, bool autoPickup = false);

    // Get all world items
    const std::vector<WorldItem>& getAllItems() const { return worldItems; }

    // Get items in range
    std::vector<uint32_t> getItemsInRange(const glm::vec3& playerPos, float range) const;

    // Remove picked-up items
    void clearPickedUpItems();

    // Render world item indicators (for UI)
    std::vector<std::pair<uint32_t, glm::vec3>> getVisibleItems(const glm::vec3& playerPos, float range) const;

private:
    std::vector<WorldItem> worldItems;
    std::shared_ptr<inventory::InventoryGrid> inventory;
    inventory::ItemFactory* itemFactory = nullptr;
    uint32_t nextWorldItemId = 1;

    uint32_t generateWorldItemId();
    bool addItemToInventory(uint32_t itemId, uint32_t quantity);
};
