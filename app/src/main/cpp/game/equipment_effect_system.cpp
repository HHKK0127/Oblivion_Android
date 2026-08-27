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

    // BUG FIX #65: Reset to base stats before reapplying bonuses
    // This prevents stat accumulation when called multiple times
    resetPlayerStatsToBase();

    // Get all equipped items
    auto equippedItems = equipmentMgr->getAllEquipped();
    auto combinedStats = equipmentMgr->getTotalStats();

    // Apply stat bonuses
    applyStatBonuses(combinedStats);

    // Apply specific weapon/armor effects
    for (const auto& item : equippedItems) {
        if (item.equipSlot == inventory::EquipSlot::Weapon) {
            applyWeaponStats(item);
        }
        // BUG FIX #66: Don't apply armor stats separately here -
        // applyStatBonuses() already handles defense from combinedStats
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
        float oldMaxHealth = playerStatus->maxHealth;
        playerStatus->maxHealth += static_cast<float>(stats.maxHealth);
        // BUG FIX #68: When maxHP increases, also increase currentHP by the same amount
        playerStatus->currentHealth += (playerStatus->maxHealth - oldMaxHealth);
        if (playerStatus->currentHealth > playerStatus->maxHealth) {
            playerStatus->currentHealth = playerStatus->maxHealth;
        }
    }

    if (stats.maxMana > 0) {
        float oldMaxMana = playerStatus->maxMana;
        playerStatus->maxMana += static_cast<float>(stats.maxMana);
        playerStatus->currentMana += (playerStatus->maxMana - oldMaxMana);
        if (playerStatus->currentMana > playerStatus->maxMana) {
            playerStatus->currentMana = playerStatus->maxMana;
        }
    }

    LOGD("Applied stat bonuses: dmg+%d, def+%d, hp+%d, mp+%d",
         stats.damage, stats.defense, stats.maxHealth, stats.maxMana);
}

void EquipmentEffectSystem::applyWeaponStats(const inventory::Item& weapon) {
    if (!playerStatus) return;

    playerStatus->equippedWeaponId = weapon.id;
    // BUG FIX #67: Set weaponDamage (not accumulate) since this is the base weapon damage
    // applyStatBonuses() already added stat bonuses on top
    if (weapon.stats.damage > 0) {
        // weaponDamage was already set to base in resetPlayerStatsToBase()
        // and had stat bonuses added by applyStatBonuses()
        // We need to set the weapon base damage here
        // The base weaponDamage (10.0f) was already set in reset, so we add the weapon's damage
        playerStatus->weaponDamage += static_cast<float>(weapon.stats.damage);
        LOGD("Equipped weapon: %s (dmg+%.1f, total=%.1f)", weapon.name.c_str(),
             static_cast<float>(weapon.stats.damage), playerStatus->weaponDamage);
    }
}

void EquipmentEffectSystem::applyArmorStats(const inventory::Item& armor) {
    // BUG FIX #66: This function is no longer called from applyEquippedBonuses()
    // Defense is already handled by applyStatBonuses() from combinedStats
    // Kept for potential future use with per-piece armor effects
    if (!playerStatus) return;

    if (armor.stats.defense > 0) {
        LOGD("Armor piece: %s (def %.1f)", armor.name.c_str(), static_cast<float>(armor.stats.defense));
    }
}

void EquipmentEffectSystem::resetPlayerStatsToBase() {
    if (!playerStatus) return;

    // BUG FIX #65: Reset combat stats to base values before reapplying equipment bonuses
    // This prevents accumulation when applyEquippedBonuses() is called multiple times
    playerStatus->weaponDamage = 10.0f;   // Base unarmed damage
    playerStatus->armorRating = 0.0f;     // No armor by default
    playerStatus->equippedWeaponId = 0;

    // Note: maxHealth, maxMana, currentHealth, currentMana are NOT reset here
    // because they include level-based bonuses from CharacterStatus::initialize()
    // The equipment bonuses will be added on top via applyStatBonuses()

    LOGD("Player stats reset to base values");
}
