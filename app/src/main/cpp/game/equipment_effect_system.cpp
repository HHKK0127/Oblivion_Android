#include "equipment_effect_system.h"
#include <algorithm>

bool EquipmentEffectSystem::initialize(std::shared_ptr<inventory::EquipmentManager> eqMgr,
                                       CharacterStatus* ps) {
    if (!eqMgr || !ps) {
        LOGE("EquipmentEffectSystem initialize failed: null pointers");
        return false;
    }
    equipmentMgr = eqMgr;
    playerStatus = ps;
    applyEquippedBonuses();
    LOGI("EquipmentEffectSystem initialized");
    return true;
}

void EquipmentEffectSystem::applyEquippedBonuses() {
    if (!equipmentMgr || !playerStatus) {
        LOGE("EquipmentEffectSystem not properly initialized");
        return;
    }

    // Get all equipped items
    auto equippedItems = equipmentMgr->getAllEquipped();
    auto combinedStats = equipmentMgr->getTotalStats();

    // Apply stat bonuses
    applyStatBonuses(combinedStats);

    // Apply specific weapon/armor effects
    for (const auto& item : equippedItems) {
        if (item.equipSlot == inventory::EquipSlot::Weapon) {
            applyWeaponStats(item);
        } else if (item.category == inventory::ItemCategory::Armor) {
            applyArmorStats(item);
        }
    }

    LOGD("Equipment bonuses applied: damage=%.1f, defense=%.1f",
         playerStatus->weaponDamage, playerStatus->armorRating);
}

void EquipmentEffectSystem::onEquipmentChanged() {
    applyEquippedBonuses();
    LOGI("Equipment changed, bonuses reapplied");
}

inventory::ItemStats EquipmentEffectSystem::getEquippedStats() const {
    if (!equipmentMgr) {
        return inventory::ItemStats();
    }
    return equipmentMgr->getTotalStats();
}

float EquipmentEffectSystem::getWeaponDamage() const {
    if (!playerStatus) return 0.0f;
    return playerStatus->weaponDamage;
}

float EquipmentEffectSystem::getArmorRating() const {
    if (!playerStatus) return 0.0f;
    return playerStatus->armorRating;
}

void EquipmentEffectSystem::applyStatBonuses(const inventory::ItemStats& stats) {
    if (!playerStatus) return;

    // Apply damage bonus
    if (stats.damage > 0) {
        playerStatus->weaponDamage += static_cast<float>(stats.damage);
    }

    // Apply defense bonus
    if (stats.defense > 0) {
        playerStatus->armorRating += static_cast<float>(stats.defense);
    }

    // Apply health/mana bonuses (add to max)
    if (stats.maxHealth > 0) {
        playerStatus->maxHealth += static_cast<float>(stats.maxHealth);
        // Optionally increase current health too
        playerStatus->currentHealth = std::min(playerStatus->currentHealth, playerStatus->maxHealth);
    }

    if (stats.maxMana > 0) {
        playerStatus->maxMana += static_cast<float>(stats.maxMana);
        playerStatus->currentMana = std::min(playerStatus->currentMana, playerStatus->maxMana);
    }

    LOGD("Applied stat bonuses: dmg+%d, def+%d, hp+%d, mp+%d",
         stats.damage, stats.defense, stats.maxHealth, stats.maxMana);
}

void EquipmentEffectSystem::applyWeaponStats(const inventory::Item& weapon) {
    if (!playerStatus) return;

    playerStatus->equippedWeaponId = weapon.id;
    if (weapon.stats.damage > 0) {
        playerStatus->weaponDamage = static_cast<float>(weapon.stats.damage);
        LOGD("Equipped weapon: %s (dmg %.1f)", weapon.name.c_str(), playerStatus->weaponDamage);
    }
}

void EquipmentEffectSystem::applyArmorStats(const inventory::Item& armor) {
    if (!playerStatus) return;

    if (armor.stats.defense > 0) {
        playerStatus->armorRating += static_cast<float>(armor.stats.defense);
        LOGD("Equipped armor: %s (def+%.1f)", armor.name.c_str(), armor.stats.defense);
    }
}
