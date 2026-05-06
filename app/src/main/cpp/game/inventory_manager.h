#pragma once

#include "inventory.h"
#include <memory>
#include <unordered_map>
#include <android/log.h>

#define LOG_TAG "InventoryManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class InventoryManager {
public:
    InventoryManager();
    ~InventoryManager();

    // Lifecycle
    bool initialize();
    void cleanup();
    void update(float deltaTime);

    // Item Database
    bool registerItem(const Item& item);
    std::shared_ptr<Item> getItemTemplate(uint32_t itemId);

    // Player Inventory
    std::shared_ptr<Inventory> getPlayerInventory() { return playerInventory; }
    bool playerAddItem(const Item& item, uint32_t quantity = 1);
    bool playerRemoveItem(uint32_t itemId, uint32_t quantity = 1);

    // Test Items
    void createTestItems();

private:
    std::shared_ptr<Inventory> playerInventory;
    std::unordered_map<uint32_t, Item> itemDatabase;  // itemId → Item template

    uint32_t nextItemId = 1;
};
