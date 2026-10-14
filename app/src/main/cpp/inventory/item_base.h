#pragma once

#include <string>
#include <cstdint>

namespace inventory {

enum class ItemCategory : uint32_t {
    Weapon     = 0,
    Armor      = 1,
    Consumable = 2,
    Material   = 3,
    Quest      = 4,
    Misc       = 5
};

// Oblivion's inventory is split into nine sections (Data\Menus\Main\inventory_menu.xml
// _weapons .. _misc) that ItemCategory cannot express on its own: "Alchemy" alone is
// apparatuses + potions + ingredients, and "Misc" is books + keys + stones + misc.
// ItemSubcategory names those nine sections. None falls back to a category default so
// items authored before this field existed still land in a sane section.
enum class ItemSubcategory : uint32_t {
    None = 0,
    Weapon,
    Armor,
    Apparatus,
    Potion,
    Ingredient,
    Book,
    Key,
    Stone,
    Misc
};

inline ItemSubcategory defaultSubcategory(ItemCategory category) {
    switch (category) {
        case ItemCategory::Weapon:     return ItemSubcategory::Weapon;
        case ItemCategory::Armor:      return ItemSubcategory::Armor;
        case ItemCategory::Consumable: return ItemSubcategory::Potion;
        case ItemCategory::Material:   return ItemSubcategory::Apparatus;
        case ItemCategory::Quest:      return ItemSubcategory::Key;
        case ItemCategory::Misc:
        default:                       return ItemSubcategory::Misc;
    }
}

enum class ItemRarity : uint32_t {
    Common    = 0,
    Uncommon  = 1,
    Rare      = 2,
    Epic      = 3,
    Legendary = 4
};

enum class EquipSlot : uint32_t {
    None     = 0,
    Head     = 1,
    Body     = 2,
    Hands    = 3,
    Feet     = 4,
    Weapon   = 5,
    Offhand  = 6,
    Accessory= 7
};

struct ItemStats {
    int damage       = 0;   // Weapon
    int defense      = 0;   // Armor
    int maxHealth    = 0;   // Bonus
    int maxMana      = 0;   // Bonus
    int strength     = 0;   // Attribute bonus
    int agility      = 0;
    int intelligence = 0;
};

struct Item {
    uint32_t    id = 0;
    std::string name;
    std::string description;
    ItemCategory category = ItemCategory::Misc;
    ItemSubcategory subcategory = ItemSubcategory::None;   // None -> defaultSubcategory(category)
    ItemRarity  rarity = ItemRarity::Common;
    EquipSlot   equipSlot = EquipSlot::None;

    float   weight = 0.0f;
    uint32_t value = 0;
    uint32_t maxStack = 1;
    uint32_t iconId = 0;       // Reference to UI icon atlas

    ItemStats stats;

    // Consumable effect (if applicable)
    int32_t healAmount = 0;
    int32_t manaAmount = 0;

    // Quest reference (if quest item)
    uint32_t questId = 0;
    
    // Enchantment fields
    uint32_t enchantmentID = 0;
    std::string enchantmentName;
    uint8_t enchantmentEffect = 0;
    float enchantmentMagnitude = 0.0f;
    uint32_t enchantmentDuration = 0;
    uint32_t maxCharges = 0;
    uint32_t currentCharges = 0;
    uint8_t enchantmentType = 0;
    
    bool isStackable() const { return maxStack > 1; }
    bool isEquippable() const { return equipSlot != EquipSlot::None; }
    bool isConsumable() const { return category == ItemCategory::Consumable; }
};

inline ItemSubcategory effectiveSubcategory(const Item& item) {
    return item.subcategory == ItemSubcategory::None ? defaultSubcategory(item.category)
                                                     : item.subcategory;
}

} // namespace inventory
