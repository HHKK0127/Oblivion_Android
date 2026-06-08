#pragma once

#include "item_base.h"
#include <array>
#include <cstdint>
#include <vector>

namespace inventory {

class EquipmentManager {
public:
    static constexpr uint32_t SLOT_COUNT = 8; // matches EquipSlot values

    EquipmentManager() = default;
    ~EquipmentManager() = default;

    // Equip / unequip
    bool equip(const Item& item);
    bool unequip(EquipSlot slot);

    // Queries
    const Item* getEquipped(EquipSlot slot) const;
    bool isSlotOccupied(EquipSlot slot) const;
    std::vector<Item> getAllEquipped() const;

    // Stat bonuses from all equipped items
    ItemStats getTotalStats() const;

    // Utility: map enum to array index
    static uint32_t slotToIndex(EquipSlot slot) { return static_cast<uint32_t>(slot); }

    void clear();

private:
    std::array<Item, SLOT_COUNT> equipped;
    std::array<bool, SLOT_COUNT> occupied{};
};

} // namespace inventory
