#pragma once

#include "../inventory/equipment_manager.h"
#include "../inventory/item_base.h"
#include "npc.h"
#include <memory>
#include <android/log.h>

#define LOG_TAG "EquipmentEffectSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * @brief 装備効果システム - 装備による能力値反映
 *
 * Phase 9B Week 4: 装備の変更をプレイヤーの能力値に反映
 * 武器ダメージ、防御力、ボーナス能力値などを管理
 */
class EquipmentEffectSystem {
public:
    EquipmentEffectSystem() = default;
    ~EquipmentEffectSystem() = default;

    // Initialize with equipment manager and character
    bool initialize(std::shared_ptr<inventory::EquipmentManager> eqMgr, CharacterStatus* playerStatus);

    // Apply all equipped items' bonuses to character
    void applyEquippedBonuses();

    // Update when equipment changes
    void onEquipmentChanged();

    // Get combined stats from all equipment
    inventory::ItemStats getEquippedStats() const;

    // Get weapon damage (including equipped weapon)
    float getWeaponDamage() const;

    // Get armor rating (combined from all armor pieces)
    float getArmorRating() const;

private:
    std::shared_ptr<inventory::EquipmentManager> equipmentMgr;
    CharacterStatus* playerStatus = nullptr;

    // Apply bonuses to character status
    void applyStatBonuses(const inventory::ItemStats& stats);
    void applyWeaponStats(const inventory::Item& weapon);
    void applyArmorStats(const inventory::Item& armor);
};
