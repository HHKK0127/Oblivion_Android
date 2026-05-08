#include "inventory.h"
#include <android/log.h>
#include <algorithm>

#define LOG_TAG "Inventory"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

Inventory::Inventory() {
    slots.resize(MAX_SLOTS);
    for (uint32_t i = 0; i < MAX_SLOTS; ++i) {
        slots[i].slotIndex = i;
    }
    LOGD("Inventory created with %d slots", MAX_SLOTS);
}

Inventory::~Inventory() {
    clear();
}

bool Inventory::addItem(const Item& item, uint32_t quantity) {
    if (quantity == 0) return false;

    // Check weight
    float itemWeight = item.weight * quantity;
    if (!canCarry(itemWeight)) {
        LOGW("Cannot carry item %s (weight %.1f kg)", item.name.c_str(), itemWeight);
        return false;
    }

    // Try to stack if item is stackable
    if (item.stackSize > 1) {
        int existingSlot = findSlotWithItem(item.itemId);
        if (existingSlot >= 0) {
            auto& slot = slots[existingSlot];
            uint32_t space = item.stackSize - slot.quantity;
            if (space > 0) {
                uint32_t toAdd = std::min(quantity, space);
                slot.quantity += toAdd;
                totalWeight += item.weight * toAdd;
                LOGD("Added %d x %s to slot %d (total: %d)",
                     toAdd, item.name.c_str(), existingSlot, slot.quantity);
                return true;
            }
        }
    }

    // Find empty slot
    int emptySlot = findEmptySlot();
    if (emptySlot < 0) {
        LOGW("No empty inventory slots available");
        return false;
    }

    // Add to empty slot
    auto& slot = slots[emptySlot];
    slot.item = item;
    slot.quantity = quantity;
    totalWeight += item.weight * quantity;

    LOGD("Added item %s to slot %d (qty: %d)", item.name.c_str(), emptySlot, quantity);
    return true;
}

bool Inventory::removeItem(uint32_t itemId, uint32_t quantity) {
    int slotIdx = findSlotWithItem(itemId);
    if (slotIdx < 0) {
        LOGW("Item %u not found in inventory", itemId);
        return false;
    }

    auto& slot = slots[slotIdx];
    if (slot.quantity < quantity) {
        LOGW("Not enough items to remove (have: %d, need: %d)",
             slot.quantity, quantity);
        return false;
    }

    totalWeight -= slot.item.weight * quantity;
    slot.quantity -= quantity;

    if (slot.quantity == 0) {
        slot.item = Item();  // Clear slot
    }

    LOGD("Removed %d x item %u from slot %d", quantity, itemId, slotIdx);
    return true;
}

bool Inventory::moveItem(uint32_t fromSlot, uint32_t toSlot) {
    if (fromSlot >= MAX_SLOTS || toSlot >= MAX_SLOTS) {
        LOGW("Invalid slot index");
        return false;
    }

    auto& from = slots[fromSlot];
    auto& to = slots[toSlot];

    if (from.isEmpty()) {
        LOGW("Source slot %d is empty", fromSlot);
        return false;
    }

    // Swap or merge logic
    if (to.isEmpty()) {
        // Simple move
        to = from;
        to.slotIndex = toSlot;
        from = InventorySlot();
        from.slotIndex = fromSlot;
    } else if (to.item.itemId == from.item.itemId &&
               from.item.stackSize > 1) {
        // Merge stacks
        uint32_t space = from.item.stackSize - to.quantity;
        uint32_t toMove = std::min(from.quantity, space);
        to.quantity += toMove;
        from.quantity -= toMove;

        if (from.quantity == 0) {
            from = InventorySlot();
            from.slotIndex = fromSlot;
        }
    } else {
        // Swap items
        std::swap(from, to);
        from.slotIndex = fromSlot;
        to.slotIndex = toSlot;
    }

    LOGD("Moved item from slot %d to slot %d", fromSlot, toSlot);
    return true;
}

int Inventory::findSlotWithItem(uint32_t itemId) const {
    for (size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].item.itemId == itemId && !slots[i].isEmpty()) {
            return i;
        }
    }
    return -1;
}

std::vector<int> Inventory::findAllSlots(uint32_t itemId) const {
    std::vector<int> result;
    for (size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].item.itemId == itemId && !slots[i].isEmpty()) {
            result.push_back(i);
        }
    }
    return result;
}

uint32_t Inventory::getItemQuantity(uint32_t itemId) const {
    uint32_t total = 0;
    for (const auto& slot : slots) {
        if (slot.item.itemId == itemId) {
            total += slot.quantity;
        }
    }
    return total;
}

bool Inventory::hasItem(uint32_t itemId) const {
    return findSlotWithItem(itemId) >= 0;
}

float Inventory::getTotalWeight() const {
    return totalWeight;
}

float Inventory::getRemainingCapacity() const {
    return MAX_WEIGHT - totalWeight;
}

bool Inventory::canCarry(float itemWeight) const {
    return (totalWeight + itemWeight) <= MAX_WEIGHT;
}

const InventorySlot& Inventory::getSlot(uint32_t slotIndex) const {
    if (slotIndex >= slots.size()) {
        static InventorySlot empty;
        return empty;
    }
    return slots[slotIndex];
}

InventorySlot& Inventory::getSlotMut(uint32_t slotIndex) {
    if (slotIndex >= slots.size()) {
        static InventorySlot empty;
        return empty;
    }
    return slots[slotIndex];
}

void Inventory::clear() {
    for (auto& slot : slots) {
        slot = InventorySlot();
    }
    totalWeight = 0.0f;
    LOGD("Inventory cleared");
}

int Inventory::findEmptySlot() const {
    for (size_t i = 0; i < slots.size(); ++i) {
        if (slots[i].isEmpty()) {
            return i;
        }
    }
    return -1;
}

void Inventory::logInventory() const {
    LOGI("=== Inventory Status ===");
    LOGI("Weight: %.1f / %.1f kg", totalWeight, MAX_WEIGHT);
    uint32_t emptySlots = 0;
    for (const auto& slot : slots) {
        if (slot.isEmpty()) {
            emptySlots++;
        } else {
            LOGI("  [%u] %s x%u (weight: %.1f)",
                 slot.slotIndex, slot.item.name.c_str(),
                 slot.quantity, slot.item.weight * slot.quantity);
        }
    }
    LOGI("Empty slots: %d / %d", emptySlots, MAX_SLOTS);
}
