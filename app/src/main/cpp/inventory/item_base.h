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

    bool isStackable() const { return maxStack > 1; }
    bool isEquippable() const { return equipSlot != EquipSlot::None; }
    bool isConsumable() const { return category == ItemCategory::Consumable; }
};

} // namespace inventory
