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
 * @brief 消費アイテムシステム - ポーション使用とアイテム効果
 *
 * Phase 9B Week 4: アイテムの使用による効果適用
 * ポーション（HP/MP回復）やスクロール（スペル発動）等の処理
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
