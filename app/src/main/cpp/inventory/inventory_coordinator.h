#pragma once

#include "inventory_grid.h"
#include "equipment_manager.h"
#include "item_factory.h"
#include <memory>

namespace inventory {

/**
 * @brief インベントリコーディネーター - インベントリシステムの統合管理
 *
 * Phase 9B: InventoryGrid + EquipmentManager + ItemFactory を統合して
 * プレイヤーのアイテム管理を一括処理
 *
 * (game/InventoryManager と区別するため InventoryCoordinator と命名)
 */
class InventoryCoordinator {
public:
    InventoryCoordinator();
    ~InventoryCoordinator() = default;

    // Lifecycle
    bool initialize();
    void cleanup();

    // Access to sub-systems
    InventoryGrid* getGrid() { return gridInventory.get(); }
    EquipmentManager* getEquipment() { return equipmentMgr.get(); }
    ItemFactory* getItemFactory() { return &itemFactory; }

    const InventoryGrid* getGrid() const { return gridInventory.get(); }
    const EquipmentManager* getEquipment() const { return equipmentMgr.get(); }

    // Helper: Add item by ID from factory
    bool addItemById(uint32_t itemId, uint32_t quantity = 1);

    // Helper: Get total stats (equipment + items)
    ItemStats getTotalStats() const;

    // Test/Debug: Populate with sample items
    void populateTestInventory();

private:
    std::unique_ptr<InventoryGrid> gridInventory;
    std::unique_ptr<EquipmentManager> equipmentMgr;
    ItemFactory itemFactory;
};

} // namespace inventory
