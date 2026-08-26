#pragma once

#include <cstdint>
#include <vector>
#include "../assets/esm_reader.h"
#include "../inventory/item_base.h"

namespace oblivion {

/**
 * @brief Converts ESM ClothingData to inventory Items
 *
 * Handles conversion of clothing records from ESM into
 * the inventory system's Item format.
 */
class ClothingConverter {
public:
    ClothingConverter();
    ~ClothingConverter();

    /**
     * @brief Initialize with ESM manager
     * @param esmMgr ESM manager with loaded data
     */
    void initialize(const ESMManager* esmMgr);

    /**
     * @brief Convert a single clothing record to an Item
     * @param clothing ClothingData from ESM
     * @return Converted Item
     */
    inventory::Item convertToItem(const ClothingData& clothing) const;

    /**
     * @brief Convert all clothing records to Items
     * @return Vector of converted Items
     */
    std::vector<inventory::Item> convertAllClothing() const;

    /**
     * @brief Get clothing by formID
     * @param formID FormID of the clothing
     * @return Converted Item, or nullptr if not found
     */
    inventory::Item* getClothingByFormID(uint32_t formID) const;

private:
    const ESMManager* esmManager = nullptr;

    /**
     * @brief Determine equip slot from clothing type
     * @param clothing ClothingData to analyze
     * @return Appropriate EquipSlot
     */
    inventory::EquipSlot determineEquipSlot(const ClothingData& clothing) const;
};

} // namespace oblivion
