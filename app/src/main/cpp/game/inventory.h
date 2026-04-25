#pragma once

#include "item.h"
#include <vector>
#include <memory>
#include <unordered_map>

class Inventory {
public:
    // Constants
    static constexpr uint32_t GRID_COLUMNS = 10;
    static constexpr uint32_t GRID_ROWS = 6;
    static constexpr uint32_t MAX_SLOTS = GRID_COLUMNS * GRID_ROWS;  // 60
    static constexpr float MAX_WEIGHT = 100.0f;  // kg

    Inventory();
    ~Inventory();

    // Item Management
    bool addItem(const Item& item, uint32_t quantity = 1);
    bool removeItem(uint32_t itemId, uint32_t quantity = 1);
    bool moveItem(uint32_t fromSlot, uint32_t toSlot);

    // Query
    int findSlotWithItem(uint32_t itemId) const;
    std::vector<int> findAllSlots(uint32_t itemId) const;
    uint32_t getItemQuantity(uint32_t itemId) const;
    bool hasItem(uint32_t itemId) const;

    // Weight
    float getTotalWeight() const;
    float getRemainingCapacity() const;
    bool canCarry(float itemWeight) const;

    // Grid Access
    const InventorySlot& getSlot(uint32_t slotIndex) const;
    InventorySlot& getSlotMut(uint32_t slotIndex);
    const std::vector<InventorySlot>& getAllSlots() const { return slots; }

    // Clear
    void clear();

    // Debug
    void logInventory() const;

private:
    std::vector<InventorySlot> slots;
    float totalWeight = 0.0f;

    int findEmptySlot() const;
    bool canStackItem(const Item& item) const;
};
