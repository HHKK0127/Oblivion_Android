#include "inventory_manager.h"

InventoryManager::InventoryManager() {
    LOGD("InventoryManager created");
}

InventoryManager::~InventoryManager() {
    cleanup();
}

bool InventoryManager::initialize() {
    playerInventory = std::make_shared<Inventory>();
    if (!playerInventory) {
        LOGE("Failed to create player inventory");
        return false;
    }

    createTestItems();
    LOGI("InventoryManager initialized");
    return true;
}

void InventoryManager::cleanup() {
    if (playerInventory) {
        playerInventory->clear();
        playerInventory = nullptr;
    }
    itemDatabase.clear();
    LOGD("InventoryManager cleaned up");
}

void InventoryManager::update(float deltaTime) {
    // Can be used for timed inventory updates (food spoilage, etc.)
}

bool InventoryManager::registerItem(const Item& item) {
    if (itemDatabase.find(item.itemId) != itemDatabase.end()) {
        LOGW("Item %u already registered", item.itemId);
        return false;
    }

    itemDatabase[item.itemId] = item;
    LOGD("Registered item: %u - %s", item.itemId, item.name.c_str());
    return true;
}

std::shared_ptr<Item> InventoryManager::getItemTemplate(uint32_t itemId) {
    auto it = itemDatabase.find(itemId);
    if (it == itemDatabase.end()) {
        LOGW("Item %u not found in database", itemId);
        return nullptr;
    }
    return std::make_shared<Item>(it->second);
}

bool InventoryManager::playerAddItem(const Item& item, uint32_t quantity) {
    if (!playerInventory) return false;
    return playerInventory->addItem(item, quantity);
}

bool InventoryManager::playerRemoveItem(uint32_t itemId, uint32_t quantity) {
    if (!playerInventory) return false;
    return playerInventory->removeItem(itemId, quantity);
}

void InventoryManager::createTestItems() {
    // Weapons
    registerItem(Item(1, "Iron Sword", ItemType::WEAPON, 12.5f, 100, 1));
    registerItem(Item(2, "Steel Dagger", ItemType::WEAPON, 5.0f, 50, 1));

    // Armor
    registerItem(Item(101, "Iron Helmet", ItemType::ARMOR, 8.0f, 75, 1));
    registerItem(Item(102, "Iron Cuirass", ItemType::ARMOR, 25.0f, 150, 1));
    registerItem(Item(103, "Iron Boots", ItemType::ARMOR, 10.0f, 60, 1));

    // Potions (stackable)
    registerItem(Item(201, "Health Potion", ItemType::POTION, 0.5f, 20, 99));
    registerItem(Item(202, "Mana Potion", ItemType::POTION, 0.5f, 20, 99));
    registerItem(Item(203, "Stamina Potion", ItemType::POTION, 0.5f, 15, 99));

    // Ingredients
    registerItem(Item(301, "Iron Ore", ItemType::INGREDIENT, 2.0f, 5, 99));
    registerItem(Item(302, "Bloodgrass", ItemType::INGREDIENT, 0.2f, 10, 99));

    // Miscellaneous
    registerItem(Item(401, "Gold Coin", ItemType::MISC, 0.01f, 1, 999));
    registerItem(Item(402, "Lockpick", ItemType::MISC, 0.1f, 5, 99));

    LOGI("Test items created: %zu items registered", itemDatabase.size());

    // Add test items to player inventory
    playerInventory->addItem(Item(1, "Iron Sword", ItemType::WEAPON, 12.5f, 100, 1), 1);
    playerInventory->addItem(Item(201, "Health Potion", ItemType::POTION, 0.5f, 20, 99), 5);
    playerInventory->addItem(Item(402, "Lockpick", ItemType::MISC, 0.1f, 5, 99), 3);

    LOGI("Test items added to player inventory");
}
