#include "inventory_coordinator.h"
#include <android/log.h>

#define LOG_TAG "InventoryCoordinator"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace inventory {

InventoryCoordinator::InventoryCoordinator()
    : gridInventory(std::make_unique<InventoryGrid>(150.0f)),
      equipmentMgr(std::make_unique<EquipmentManager>()) {
    LOGD("InventoryCoordinator created");
}

bool InventoryCoordinator::initialize() {
    if (!gridInventory || !equipmentMgr) {
        LOGE("InventoryCoordinator initialization failed: null pointers");
        return false;
    }
    LOGI("InventoryCoordinator initialized");
    return true;
}

void InventoryCoordinator::cleanup() {
    if (gridInventory) gridInventory->clear();
    if (equipmentMgr) equipmentMgr->clear();
    LOGD("InventoryCoordinator cleaned up");
}

bool InventoryCoordinator::addItemById(uint32_t itemId, uint32_t quantity) {
    Item item = itemFactory.createItem(itemId, quantity);
    if (item.id == 0) {
        LOGW("Item ID %u not found in factory", itemId);
        return false;
    }
    if (!gridInventory->addItem(item, quantity)) {
        LOGW("Failed to add item %s to inventory", item.name.c_str());
        return false;
    }
    LOGD("Added %d x %s (ID: %u)", quantity, item.name.c_str(), itemId);
    return true;
}

ItemStats InventoryCoordinator::getTotalStats() const {
    ItemStats total;
    if (equipmentMgr) {
        total = equipmentMgr->getTotalStats();
    }
    // Could add inventory item bonuses here in future
    return total;
}

void InventoryCoordinator::populateTestInventory() {
    LOGI("Populating test inventory");

    // Add some weapons
    addItemById(ItemFactory::ITEM_ID_IRON_SWORD, 1);

    // Add some armor
    addItemById(ItemFactory::ITEM_ID_IRON_ARMOR, 1);

    // Add consumables
    addItemById(ItemFactory::ITEM_ID_HEALTH_POTION, 5);
    addItemById(ItemFactory::ITEM_ID_MANA_POTION, 3);

    // Add materials
    addItemById(ItemFactory::ITEM_ID_IRON_ORE, 10);
    addItemById(ItemFactory::ITEM_ID_LEATHER, 8);

    // Add scroll
    addItemById(ItemFactory::ITEM_ID_SCROLL_SHIELD, 1);

    LOGI("Test inventory populated with %d items", gridInventory->getUsedSlots());
}

} // namespace inventory
