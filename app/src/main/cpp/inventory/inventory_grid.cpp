#include "inventory_grid.h"
#include <algorithm>
#include <numeric>

namespace inventory {

InventoryGrid::InventoryGrid(float maxW) : maxWeight(maxW) {
    slots.resize(MAX_SLOTS);
}

bool InventoryGrid::addItem(const Item& item, uint32_t quantity) {
    if (quantity == 0) return false;
    float totalWeight = item.weight * static_cast<float>(quantity);
    if (!canCarry(totalWeight)) return false;

    uint32_t remaining = quantity;

    if (item.isStackable()) {
        uint32_t stacked = tryStack(item, remaining);
        remaining -= stacked;
        if (remaining == 0) {
            recalculateWeight();
            return true;
        }
    }

    // Non-stackable or excess: place into empty slots one by one
    while (remaining > 0) {
        int empty = findEmptySlot();
        if (empty < 0) return false; // no space

        uint32_t place = item.isStackable() ? std::min(remaining, item.maxStack) : 1;
        slots[empty].item = item;
        slots[empty].quantity = place;
        remaining -= place;
    }

    recalculateWeight();
    return true;
}

bool InventoryGrid::removeItem(uint32_t itemId, uint32_t quantity) {
    if (quantity == 0) return false;
    uint32_t remaining = quantity;
    for (auto& slot : slots) {
        if (slot.isEmpty() || slot.item.id != itemId) continue;
        uint32_t take = std::min(remaining, slot.quantity);
        slot.quantity -= take;
        if (slot.quantity == 0) {
            slot.item = Item();
        }
        remaining -= take;
        if (remaining == 0) break;
    }
    recalculateWeight();
    return remaining == 0;
}

bool InventoryGrid::removeSlot(uint32_t slotIndex, uint32_t quantity) {
    if (slotIndex >= MAX_SLOTS) return false;
    auto& slot = slots[slotIndex];
    if (slot.isEmpty()) return false;
    uint32_t take = std::min(quantity, slot.quantity);
    slot.quantity -= take;
    if (slot.quantity == 0) {
        slot.item = Item();
    }
    recalculateWeight();
    return true;
}

bool InventoryGrid::moveSlot(uint32_t fromIndex, uint32_t toIndex) {
    if (fromIndex >= MAX_SLOTS || toIndex >= MAX_SLOTS) return false;
    if (fromIndex == toIndex) return true;

    auto& from = slots[fromIndex];
    auto& to   = slots[toIndex];

    if (from.isEmpty()) return false;

    // Try stack if same item and stackable
    if (!to.isEmpty() && to.item.id == from.item.id && from.item.isStackable()) {
        uint32_t canAccept = to.remainingStack();
        if (canAccept > 0) {
            uint32_t moveAmt = std::min(from.quantity, canAccept);
            to.quantity += moveAmt;
            from.quantity -= moveAmt;
            if (from.quantity == 0) from.item = Item();
            recalculateWeight();
            return true;
        }
    }

    // Simple move (overwrite target if rules allow)
    if (!to.isEmpty() && to.item.id != from.item.id) {
        // Swap instead of overwrite to avoid loss
        std::swap(from, to);
        recalculateWeight();
        return true;
    }

    // Move into empty
    if (to.isEmpty()) {
        to = from;
        from = GridSlot();
        recalculateWeight();
        return true;
    }

    return false;
}

bool InventoryGrid::swapSlots(uint32_t a, uint32_t b) {
    if (a >= MAX_SLOTS || b >= MAX_SLOTS) return false;
    std::swap(slots[a], slots[b]);
    recalculateWeight();
    return true;
}

bool InventoryGrid::canDropHere(uint32_t slotIndex, const Item& item) const {
    if (slotIndex >= MAX_SLOTS) return false;
    const auto& slot = slots[slotIndex];
    if (slot.isEmpty()) return true;
    if (slot.item.id == item.id && item.isStackable() && slot.remainingStack() > 0) return true;
    return false;
}

bool InventoryGrid::splitStack(uint32_t slotIndex, uint32_t takeAmount, uint32_t targetSlot) {
    if (slotIndex >= MAX_SLOTS || targetSlot >= MAX_SLOTS) return false;
    auto& src = slots[slotIndex];
    if (src.isEmpty() || !src.item.isStackable()) return false;
    if (takeAmount == 0 || takeAmount >= src.quantity) return false;
    auto& dst = slots[targetSlot];
    if (!dst.isEmpty()) return false;

    dst.item = src.item;
    dst.quantity = takeAmount;
    src.quantity -= takeAmount;
    recalculateWeight();
    return true;
}

uint32_t InventoryGrid::getItemCount(uint32_t itemId) const {
    uint32_t total = 0;
    for (const auto& s : slots) {
        if (!s.isEmpty() && s.item.id == itemId) total += s.quantity;
    }
    return total;
}

bool InventoryGrid::hasItem(uint32_t itemId) const {
    for (const auto& s : slots) {
        if (!s.isEmpty() && s.item.id == itemId) return true;
    }
    return false;
}

int InventoryGrid::findFirstSlot(uint32_t itemId) const {
    for (uint32_t i = 0; i < MAX_SLOTS; ++i) {
        if (!slots[i].isEmpty() && slots[i].item.id == itemId) return static_cast<int>(i);
    }
    return -1;
}

int InventoryGrid::findEmptySlot() const {
    for (uint32_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots[i].isEmpty()) return static_cast<int>(i);
    }
    return -1;
}

bool InventoryGrid::isFull() const {
    return findEmptySlot() < 0;
}

uint32_t InventoryGrid::getUsedSlots() const {
    uint32_t used = 0;
    for (const auto& s : slots) {
        if (!s.isEmpty()) ++used;
    }
    return used;
}

void InventoryGrid::sort(SortMode mode) {
    // Extract non-empty slots
    std::vector<GridSlot> filled;
    for (auto& s : slots) {
        if (!s.isEmpty()) filled.push_back(s);
    }

    switch (mode) {
        case SortMode::Name:
            std::sort(filled.begin(), filled.end(),
                [](const GridSlot& a, const GridSlot& b) { return a.item.name < b.item.name; });
            break;
        case SortMode::Category:
            std::sort(filled.begin(), filled.end(),
                [](const GridSlot& a, const GridSlot& b) { return a.item.category < b.item.category; });
            break;
        case SortMode::Rarity:
            std::sort(filled.begin(), filled.end(),
                [](const GridSlot& a, const GridSlot& b) { return a.item.rarity > b.item.rarity; });
            break;
        case SortMode::Weight:
            std::sort(filled.begin(), filled.end(),
                [](const GridSlot& a, const GridSlot& b) { return a.item.weight < b.item.weight; });
            break;
        case SortMode::Value:
            std::sort(filled.begin(), filled.end(),
                [](const GridSlot& a, const GridSlot& b) { return a.item.value > b.item.value; });
            break;
    }

    // Reconstruct
    clear();
    for (size_t i = 0; i < filled.size(); ++i) {
        slots[i] = filled[i];
    }
    recalculateWeight();
}

std::vector<uint32_t> InventoryGrid::getSlotsByCategory(ItemCategory category) const {
    std::vector<uint32_t> result;
    for (uint32_t i = 0; i < MAX_SLOTS; ++i) {
        if (!slots[i].isEmpty() && slots[i].item.category == category) result.push_back(i);
    }
    return result;
}

void InventoryGrid::clear() {
    for (auto& s : slots) s = GridSlot();
    currentWeight = 0.0f;
}

void InventoryGrid::recalculateWeight() {
    currentWeight = 0.0f;
    for (const auto& s : slots) {
        if (!s.isEmpty()) currentWeight += s.item.weight * static_cast<float>(s.quantity);
    }
}

uint32_t InventoryGrid::tryStack(const Item& item, uint32_t quantity) {
    uint32_t placed = 0;
    for (auto& slot : slots) {
        if (slot.isEmpty() || slot.item.id != item.id) continue;
        uint32_t canAdd = slot.remainingStack();
        if (canAdd == 0) continue;
        uint32_t add = std::min(quantity - placed, canAdd);
        slot.quantity += add;
        placed += add;
        if (placed == quantity) break;
    }
    return placed;
}

bool InventoryGrid::tryPlaceInEmpty(const Item& item, uint32_t quantity) {
    uint32_t remaining = quantity;
    while (remaining > 0) {
        int idx = findEmptySlot();
        if (idx < 0) return false;
        uint32_t place = item.isStackable() ? std::min(remaining, item.maxStack) : 1;
        slots[idx].item = item;
        slots[idx].quantity = place;
        remaining -= place;
    }
    return true;
}

} // namespace inventory
