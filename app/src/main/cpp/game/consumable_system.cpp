#include "consumable_system.h"

bool ConsumableSystem::initialize(std::shared_ptr<inventory::InventoryGrid> inv, CharacterStatus* ps) {
    if (!inv || !ps) {
        LOGE("ConsumableSystem initialize failed: null inventory or player status");
        return false;
    }
    inventory = inv;
    playerStatus = ps;
    LOGI("ConsumableSystem initialized");
    return true;
}

bool ConsumableSystem::useItem(uint32_t slotIndex) {
    if (!inventory || !playerStatus) {
        LOGE("ConsumableSystem not properly initialized");
        return false;
    }

    if (slotIndex >= inventory::InventoryGrid::MAX_SLOTS) {
        LOGE("Invalid slot index: %u", slotIndex);
        return false;
    }

    const auto& slot = inventory->getSlot(slotIndex);
    if (slot.isEmpty()) {
        LOGE("Slot %u is empty", slotIndex);
        return false;
    }

    if (!slot.item.isConsumable()) {
        LOGE("Item %s is not consumable", slot.item.name.c_str());
        return false;
    }

    // Apply effect before removing
    applyConsumableEffect(slot.item, *playerStatus);

    // Remove from inventory
    if (!inventory->removeSlot(slotIndex, 1)) {
        LOGE("Failed to remove item from slot %u", slotIndex);
        return false;
    }

    LOGI("Used item: %s (slot %u)", slot.item.name.c_str(), slotIndex);
    return true;
}

bool ConsumableSystem::useItemById(uint32_t itemId) {
    if (!inventory) return false;

    int slotIndex = inventory->findFirstSlot(itemId);
    if (slotIndex < 0) {
        LOGD("Item ID %u not found in inventory", itemId);
        return false;
    }

    return useItem(static_cast<uint32_t>(slotIndex));
}

bool ConsumableSystem::isItemConsumable(uint32_t slotIndex) const {
    if (!inventory || slotIndex >= inventory::InventoryGrid::MAX_SLOTS) {
        return false;
    }

    const auto& slot = inventory->getSlot(slotIndex);
    return !slot.isEmpty() && slot.item.isConsumable();
}

void ConsumableSystem::applyConsumableEffect(const inventory::Item& item, CharacterStatus& target) {
    if (item.healAmount > 0) {
        applyHealthRecovery(item, target);
    }

    if (item.manaAmount > 0) {
        applyManaRecovery(item, target);
    }

    // Additional effects can be added here
    // e.g., temporary stat boosts, status effects, etc.
}

void ConsumableSystem::applyHealthRecovery(const inventory::Item& item, CharacterStatus& target) {
    float oldHealth = target.currentHealth;
    target.heal(static_cast<float>(item.healAmount));
    float healed = target.currentHealth - oldHealth;

    LOGI("Health recovery: %s restored %.0f HP (%.0f -> %.0f / %.0f)",
         item.name.c_str(), healed, oldHealth, target.currentHealth, target.maxHealth);

    // Could trigger audio/visual effects here
}

void ConsumableSystem::applyManaRecovery(const inventory::Item& item, CharacterStatus& target) {
    float oldMana = target.currentMana;
    target.currentMana = std::min(target.maxMana, target.currentMana + static_cast<float>(item.manaAmount));
    float recovered = target.currentMana - oldMana;

    LOGI("Mana recovery: %s restored %.0f MP (%.0f -> %.0f / %.0f)",
         item.name.c_str(), recovered, oldMana, target.currentMana, target.maxMana);
}
