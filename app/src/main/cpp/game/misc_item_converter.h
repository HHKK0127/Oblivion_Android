#pragma once

#include "../assets/esm_reader.h"
#include "../inventory/item_base.h"
#include <vector>

namespace oblivion {

/// Misc item converter for inventory integration
class MiscItemConverter {
public:
    MiscItemConverter();
    ~MiscItemConverter();

    void initialize(const ESMManager* esmMgr);

    /// Convert a single misc item to inventory item
    inventory::Item convertToItem(const MiscItemData& misc) const;

    /// Convert all misc items to inventory items
    std::vector<inventory::Item> convertAllMiscItems() const;

    /// Get misc item by formID
    inventory::Item* getMiscItemByFormID(uint32_t formID) const;

private:
    const ESMManager* esmManager = nullptr;
};

} // namespace oblivion
