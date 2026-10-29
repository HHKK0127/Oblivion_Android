#include "item_catalog.h"

#include <cstddef>
#include <cstdio>
#include <utility>

#include "inventory.h"
#include "../assets/esm_reader.h"

namespace game {

namespace {

void applyName(ItemInfo& info, const std::string& fullName,
               const std::string& editorID) {
    info.name = fullName.empty() ? editorID : fullName;
}

void applyResolved(ItemInfo& info, const std::string& fullName,
                   const std::string& editorID, ItemType type, float weight,
                   int32_t value) {
    applyName(info, fullName, editorID);
    info.type = type;
    info.weight = weight;
    info.value = value;
    info.resolved = true;
}

}  // namespace

ItemInfo goldItemInfo() {
    ItemInfo info;
    info.formID = kGoldFormID;
    info.name = "Gold";
    info.type = ItemType::MISC;
    info.weight = 0.0f;
    info.value = 1;
    info.resolved = true;
    return info;
}

std::string itemIdFromFormID(uint32_t formID) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%08X", formID);
    return std::string(buffer);
}

uint32_t formIDFromItemId(const std::string& itemId) {
    // Only the "XXXXXXXX" form produced above round-trips; anything else is not
    // a base item FormID and maps to 0 (no item).
    if (itemId.empty() || itemId.size() > 8) return 0;

    uint32_t value = 0;
    for (char c : itemId) {
        value <<= 4;
        if (c >= '0' && c <= '9') {
            value |= static_cast<uint32_t>(c - '0');
        } else if (c >= 'A' && c <= 'F') {
            value |= static_cast<uint32_t>(c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            value |= static_cast<uint32_t>(c - 'a' + 10);
        } else {
            return 0;
        }
    }
    return value;
}

ItemInfo resolveItemInfo(const oblivion::ESMManager& esm, uint32_t formID) {
    ItemInfo info;
    info.formID = formID;
    if (formID == 0) {
        // A null base record is not an unknown item, it is no item at all.
        return info;
    }
    if (formID == kGoldFormID) {
        return goldItemInfo();
    }

    // Shop stock spans every carryable record type. The order mirrors
    // Container::populateFromESM so both paths resolve a shared FormID the same
    // way, then extends it with the records only the shop path needs.
    if (const oblivion::WeaponData* rec = esm.findWeapon(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::WEAPON,
                      static_cast<float>(rec->weight),
                      static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::ArmorData* rec = esm.findArmor(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::ARMOR,
                      static_cast<float>(rec->weight),
                      static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::ClothingData* rec = esm.findClothing(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::CLOTHING,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::BookData* rec = esm.findBook(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::BOOK,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::AlchemyData* rec = esm.findAlchemy(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::POTION,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::IngredientData* rec = esm.findIngredient(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::INGREDIENT,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::MiscItemData* rec = esm.findMiscItem(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    // ItemType has no dedicated category for the record types below, so they
    // trade as MISC just like they sort in the vanilla inventory.
    if (const oblivion::KeyData* rec = esm.findKey(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::AmmoData* rec = esm.findAmmo(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::SigilStoneData* rec = esm.findSigilStone(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::SoulGemData* rec = esm.findSoulGem(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, static_cast<int32_t>(rec->value));
        return info;
    }
    if (const oblivion::ApparatusData* rec = esm.findApparatus(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, rec->value);
        return info;
    }
    if (const oblivion::LightData* rec = esm.findLight(formID)) {
        applyResolved(info, rec->fullName, rec->editorID, ItemType::MISC,
                      rec->weight, rec->value);
        return info;
    }

    // Unknown base record: keep a stable placeholder so a barter row still has
    // a name and a price, and let resolved == false mark it as a data gap.
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Item_%08X", formID);
    info.name = buffer;
    info.type = ItemType::MISC;
    info.weight = 0.1f;
    info.value = 1;
    info.resolved = false;
    return info;
}

int32_t countPlayerItem(const Inventory& inventory, uint32_t formID) {
    const uint64_t total = inventory.getItemQuantity(formID);
    if (total > 0x7FFFFFFF) return 0x7FFFFFFF;
    return static_cast<int32_t>(total);
}

bool addPlayerItem(Inventory& inventory, const ItemInfo& info, int32_t count) {
    if (count <= 0) return false;

    Item item;
    item.itemId = info.formID;
    item.name = info.name;
    item.type = info.type;
    item.weight = info.weight;
    item.value = info.value > 0 ? static_cast<uint32_t>(info.value) : 0u;
    item.stackSize = static_cast<uint32_t>(count);
    return inventory.addItem(item, static_cast<uint32_t>(count));
}

bool removePlayerItem(Inventory& inventory, uint32_t formID, int32_t count) {
    if (count <= 0) return false;
    return inventory.removeItem(formID, static_cast<uint32_t>(count));
}

int32_t playerGold(const Inventory& inventory) {
    return countPlayerItem(inventory, kGoldFormID);
}

}  // namespace game