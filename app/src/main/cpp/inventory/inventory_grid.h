#pragma once

#include "item_base.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace inventory {

struct GridSlot {
    Item     item;
    uint32_t quantity = 0;
    bool     locked = false;   // For UI drag-and-drop ghost slots

    bool isEmpty() const { return quantity == 0; }
    uint32_t remainingStack() const {
        if (isEmpty()) return 0;
        return (item.maxStack > quantity) ? (item.maxStack - quantity) : 0;
    }
};

class InventoryGrid {
public:
    static constexpr uint32_t COLUMNS = 10;
    static constexpr uint32_t ROWS    = 6;
    static constexpr uint32_t MAX_SLOTS = COLUMNS * ROWS; // 60

    explicit InventoryGrid(float maxWeight = 150.0f);
    ~InventoryGrid() = default;

    // Core operations
    bool addItem(const Item& item, uint32_t quantity = 1);
    bool removeItem(uint32_t itemId, uint32_t quantity = 1);
    bool removeSlot(uint32_t slotIndex, uint32_t quantity = 1);

    // Move / swap slots (for drag-and-drop)
    bool moveSlot(uint32_t fromIndex, uint32_t toIndex);
    bool swapSlots(uint32_t a, uint32_t b);
    bool canDropHere(uint32_t slotIndex, const Item& item) const;

    // Split stack (long-press menu)
    bool splitStack(uint32_t slotIndex, uint32_t takeAmount, uint32_t targetSlot);

    // Queries
    uint32_t getItemCount(uint32_t itemId) const;
    bool hasItem(uint32_t itemId) const;
    int findFirstSlot(uint32_t itemId) const;
    int findEmptySlot() const;
    const GridSlot& getSlot(uint32_t index) const { return slots[index]; }
    GridSlot& getSlotMut(uint32_t index) { return slots[index]; }
    const std::vector<GridSlot>& getAllSlots() const { return slots; }

    // Weight
    float getTotalWeight() const { return currentWeight; }
    float getMaxWeight() const { return maxWeight; }
    float getRemainingWeight() const { return maxWeight - currentWeight; }
    bool canCarry(float weight) const { return (currentWeight + weight) <= maxWeight; }

    // Capacity
    bool isFull() const;
    uint32_t getUsedSlots() const;
    uint32_t getFreeSlots() const { return MAX_SLOTS - getUsedSlots(); }

    // Sorting
    enum class SortMode {
        Name,
        Category,
        Rarity,
        Weight,
        Value
    };
    void sort(SortMode mode);

    // Filter for UI (non-destructive view)
    std::vector<uint32_t> getSlotsByCategory(ItemCategory category) const;

    // Clear
    void clear();

private:
    std::vector<GridSlot> slots;
    float currentWeight = 0.0f;
    float maxWeight;

    void recalculateWeight();
    uint32_t tryStack(const Item& item, uint32_t quantity);
    bool tryPlaceInEmpty(const Item& item, uint32_t quantity);
};

} // namespace inventory
