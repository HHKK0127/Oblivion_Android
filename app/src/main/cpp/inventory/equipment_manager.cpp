#include "equipment_manager.h"

namespace inventory {

bool EquipmentManager::equip(const Item& item) {
    if (!item.isEquippable()) return false;
    uint32_t idx = slotToIndex(item.equipSlot);
    if (idx >= SLOT_COUNT) return false;
    equipped[idx] = item;
    occupied[idx] = true;
    return true;
}

bool EquipmentManager::unequip(EquipSlot slot) {
    uint32_t idx = slotToIndex(slot);
    if (idx >= SLOT_COUNT) return false;
    if (!occupied[idx]) return false;
    equipped[idx] = Item();
    occupied[idx] = false;
    return true;
}

const Item* EquipmentManager::getEquipped(EquipSlot slot) const {
    uint32_t idx = slotToIndex(slot);
    if (idx >= SLOT_COUNT || !occupied[idx]) return nullptr;
    return &equipped[idx];
}

bool EquipmentManager::isSlotOccupied(EquipSlot slot) const {
    uint32_t idx = slotToIndex(slot);
    if (idx >= SLOT_COUNT) return false;
    return occupied[idx];
}

std::vector<Item> EquipmentManager::getAllEquipped() const {
    std::vector<Item> result;
    for (uint32_t i = 0; i < SLOT_COUNT; ++i) {
        if (occupied[i]) result.push_back(equipped[i]);
    }
    return result;
}

ItemStats EquipmentManager::getTotalStats() const {
    ItemStats total;
    for (uint32_t i = 0; i < SLOT_COUNT; ++i) {
        if (!occupied[i]) continue;
        const ItemStats& s = equipped[i].stats;
        total.damage       += s.damage;
        total.defense      += s.defense;
        total.maxHealth    += s.maxHealth;
        total.maxMana      += s.maxMana;
        total.strength     += s.strength;
        total.agility      += s.agility;
        total.intelligence += s.intelligence;
    }
    return total;
}

void EquipmentManager::clear() {
    for (uint32_t i = 0; i < SLOT_COUNT; ++i) {
        equipped[i] = Item();
        occupied[i] = false;
    }
}

} // namespace inventory
