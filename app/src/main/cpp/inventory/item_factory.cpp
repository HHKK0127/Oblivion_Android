#include "item_factory.h"

namespace inventory {

ItemFactory::ItemFactory() {
    initializeDefaultItems();
}

void ItemFactory::initializeDefaultItems() {
    // Weapons
    {
        Item sword;
        sword.id = ITEM_ID_IRON_SWORD;
        sword.name = "Iron Sword";
        sword.description = "A well-crafted iron sword";
        sword.category = ItemCategory::Weapon;
        sword.rarity = ItemRarity::Common;
        sword.equipSlot = EquipSlot::Weapon;
        sword.weight = 8.5f;
        sword.value = 75;
        sword.maxStack = 1;
        sword.stats.damage = 8;
        registerItem(sword);
    }

    // Armor
    {
        Item armor;
        armor.id = ITEM_ID_IRON_ARMOR;
        armor.name = "Iron Cuirass";
        armor.description = "Heavy iron chest plate";
        armor.category = ItemCategory::Armor;
        armor.rarity = ItemRarity::Common;
        armor.equipSlot = EquipSlot::Body;
        armor.weight = 12.0f;
        armor.value = 120;
        armor.maxStack = 1;
        armor.stats.defense = 12;
        registerItem(armor);
    }

    // Consumables - Health Potion
    {
        Item potion;
        potion.id = ITEM_ID_HEALTH_POTION;
        potion.name = "Health Potion";
        potion.description = "Restores 50 HP when consumed";
        potion.category = ItemCategory::Consumable;
        potion.rarity = ItemRarity::Common;
        potion.weight = 0.5f;
        potion.value = 25;
        potion.maxStack = 20;
        potion.healAmount = 50;
        registerItem(potion);
    }

    // Consumables - Mana Potion
    {
        Item potion;
        potion.id = ITEM_ID_MANA_POTION;
        potion.name = "Mana Potion";
        potion.description = "Restores 50 MP when consumed";
        potion.category = ItemCategory::Consumable;
        potion.rarity = ItemRarity::Common;
        potion.weight = 0.5f;
        potion.value = 25;
        potion.maxStack = 20;
        potion.manaAmount = 50;
        registerItem(potion);
    }

    // Materials - Iron Ore
    {
        Item ore;
        ore.id = ITEM_ID_IRON_ORE;
        ore.name = "Iron Ore";
        ore.description = "Raw iron ore for crafting";
        ore.category = ItemCategory::Material;
        ore.rarity = ItemRarity::Common;
        ore.weight = 1.0f;
        ore.value = 10;
        ore.maxStack = 50;
        registerItem(ore);
    }

    // Materials - Leather
    {
        Item leather;
        leather.id = ITEM_ID_LEATHER;
        leather.name = "Leather";
        leather.description = "Soft leather for armor crafting";
        leather.category = ItemCategory::Material;
        leather.rarity = ItemRarity::Common;
        leather.weight = 0.5f;
        leather.value = 15;
        leather.maxStack = 30;
        registerItem(leather);
    }

    // Scroll
    {
        Item scroll;
        scroll.id = ITEM_ID_SCROLL_SHIELD;
        scroll.name = "Scroll of Shielding";
        scroll.description = "Casts Shield spell when read";
        scroll.category = ItemCategory::Consumable;
        scroll.rarity = ItemRarity::Uncommon;
        scroll.weight = 0.2f;
        scroll.value = 100;
        scroll.maxStack = 1;
        registerItem(scroll);
    }
}

Item ItemFactory::createItem(uint32_t itemId, uint32_t quantity) const {
    auto it = itemDatabase.find(itemId);
    if (it != itemDatabase.end()) {
        Item item = it->second;
        // Quantity is handled by InventoryGrid, not stored in Item
        return item;
    }

    // Return empty/invalid item
    Item invalid;
    invalid.id = 0;
    invalid.name = "Unknown Item";
    return invalid;
}

void ItemFactory::registerItem(const Item& itemTemplate) {
    itemDatabase[itemTemplate.id] = itemTemplate;
}

std::vector<Item> ItemFactory::getAllItems() const {
    std::vector<Item> result;
    for (const auto& pair : itemDatabase) {
        result.push_back(pair.second);
    }
    return result;
}

} // namespace inventory
