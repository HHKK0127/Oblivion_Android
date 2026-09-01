#pragma once

#include "item_base.h"
#include "../assets/esm_reader.h"  // for ArmorData
#include <unordered_map>
#include <vector>
#include <memory>

namespace inventory {

/**
 * @brief Item database - Centralized item definition management
 *
 * Phase 9B: Provide standard item definitions for inventory system
 * Manage definitions of all items used in the game, and
 * Ensure consistency with inventory system
 */
class ItemFactory {
public:
    ItemFactory();
    ~ItemFactory() = default;

    // Initialize with default items
    static ItemFactory& getInstance() {
        static ItemFactory instance;
        return instance;
    }

    // Create item by ID
    Item createItem(uint32_t itemId, uint32_t quantity = 1) const;

    // Register custom item definition
    void registerItem(const Item& itemTemplate);

    // Import armor records from ESM data
    void loadArmorsFromESM(const oblivion::ESMManager& esmMgr);

    // Get all registered items
    std::vector<Item> getAllItems() const;

    // Item ID constants (standard items)
    static constexpr uint32_t ITEM_ID_IRON_SWORD = 1001;
    static constexpr uint32_t ITEM_ID_IRON_ARMOR = 1002;
    static constexpr uint32_t ITEM_ID_HEALTH_POTION = 2001;
    static constexpr uint32_t ITEM_ID_MANA_POTION = 2002;
    static constexpr uint32_t ITEM_ID_IRON_ORE = 3001;
    static constexpr uint32_t ITEM_ID_LEATHER = 3002;
    static constexpr uint32_t ITEM_ID_SCROLL_SHIELD = 4001;

private:
    std::unordered_map<uint32_t, Item> itemDatabase;

    void initializeDefaultItems();
};

} // namespace inventory
