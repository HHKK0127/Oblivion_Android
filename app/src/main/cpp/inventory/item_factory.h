#pragma once

#include "item_base.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace inventory {

/**
 * @brief アイテムデータベース - アイテム定義の一元管理
 *
 * Phase 9B: インベントリシステムでの標準アイテム定義を提供
 * ゲーム内で使用されるすべてのアイテムの定義を管理し、
 * インベントリシステムとの一貫性を保証
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
