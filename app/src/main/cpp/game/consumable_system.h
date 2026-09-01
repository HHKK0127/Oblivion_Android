#pragma once

#include "../inventory/inventory_grid.h"
#include "../inventory/equipment_manager.h"
#include "npc.h"
#include <memory>
#include <android/log.h>

#define LOG_TAG "ConsumableSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * @brief Consumable item system - Potion usage and item effects
 *
 * Phase 9B Week 4: Apply effects from item usage
 * Processing potions (HP/MP recovery) and scrolls (spell activation) etc.
 */
class ConsumableSystem {
public:
    ConsumableSystem() = default;
    ~ConsumableSystem() = default;

    // Initialize with inventory and character
    bool initialize(std::shared_ptr<inventory::InventoryGrid> inv, CharacterStatus* playerStatus);

    // Use item by slot index
    // Returns true if item was successfully consumed
    bool useItem(uint32_t slotIndex);

    // Use item by item ID (finds first matching item)
    bool useItemById(uint32_t itemId);

    // Check if item at slot is consumable
    bool isItemConsumable(uint32_t slotIndex) const;

    // Apply consumable effects to target
    void applyConsumableEffect(const inventory::Item& item, CharacterStatus& target);

private:
    std::shared_ptr<inventory::InventoryGrid> inventory;
    CharacterStatus* playerStatus = nullptr;

    // Effect application
    void applyHealthRecovery(const inventory::Item& item, CharacterStatus& target);
    void applyManaRecovery(const inventory::Item& item, CharacterStatus& target);
    void applyStatBonus(const inventory::Item& item, CharacterStatus& target);
};
