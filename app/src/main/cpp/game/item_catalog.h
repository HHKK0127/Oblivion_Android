#pragma once

#include <cstdint>
#include <string>

#include "item.h"

namespace oblivion { class ESMManager; }
class Inventory;

namespace game {

// Gold is an ordinary MISC inventory item in Oblivion (FormID 0x0000000F), so
// barter moves the player's purse through the same stacking path as any other
// trade instead of a separate counter.
constexpr uint32_t kGoldFormID = 0x0000000F;

// Presentation data for one base item FormID, resolved from the loaded ESM.
// resolved stays false when no record matches; the name is then a stable
// "Item_XXXXXXXX" placeholder, matching Container::populateFromESM.
struct ItemInfo {
    uint32_t formID = 0;
    std::string name;
    ItemType type = ItemType::MISC;
    float weight = 0.0f;
    int32_t value = 0;
    bool resolved = false;
};

// Resolve a base item FormID against every item record type the ESM reader
// exposes. Never returns an empty name.
ItemInfo resolveItemInfo(const oblivion::ESMManager& esm, uint32_t formID);

// The synthetic Gold entry (MISC, weightless, value 1 per coin).
ItemInfo goldItemInfo();

// Merchant stock is keyed by a string, so base item FormIDs round-trip through
// a stable 8 digit uppercase hex form ("0000000F") instead of a display name.
std::string itemIdFromFormID(uint32_t formID);
uint32_t formIDFromItemId(const std::string& itemId);

// The player inventory lives in exactly one place, the grid Inventory that
// InventoryManager owns (60 rows, 150 kg, one row per base item FormID), so the
// barter and container paths read and write that store instead of keeping a
// parallel flat vector in step. countPlayerItem and removePlayerItem agree by
// construction because both go through the same store.
int32_t countPlayerItem(const Inventory& inventory, uint32_t formID);

// Adds count of the base item, merging into an existing row for the same
// FormID. Returns false when the grid is full or the weight limit would be
// exceeded, so callers that also mutate another container can bail out without
// losing the item.
bool addPlayerItem(Inventory& inventory, const ItemInfo& info, int32_t count);
bool removePlayerItem(Inventory& inventory, uint32_t formID, int32_t count);

// Convenience wrapper: the player's current purse, in gold.
int32_t playerGold(const Inventory& inventory);

}  // namespace game
