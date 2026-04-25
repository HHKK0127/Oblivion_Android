#pragma once

#include <string>
#include <cstdint>

enum class ItemType {
    WEAPON,
    ARMOR,
    CLOTHING,
    ACCESSORY,
    INGREDIENT,
    POTION,
    SCROLL,
    BOOK,
    MISC
};

struct Item {
    uint32_t itemId;
    std::string name;
    ItemType type;
    float weight;           // kg
    uint32_t value;         // gold coins
    std::string iconPath;   // "textures/items/..."
    uint32_t stackSize;     // 1 for unique, >1 for stackable
    std::string description;

    // Constructor
    Item() : itemId(0), type(ItemType::MISC), weight(0.0f),
             value(0), stackSize(1) {}

    Item(uint32_t id, const std::string& n, ItemType t, float w,
         uint32_t v, uint32_t stack = 1)
        : itemId(id), name(n), type(t), weight(w), value(v), stackSize(stack) {}
};

struct InventorySlot {
    Item item;
    uint32_t quantity = 0;      // Number of items in this slot
    uint32_t slotIndex = 0;     // Grid position (0-59 for 6x10 grid)

    bool isEmpty() const { return quantity == 0; }
};
