#include "barter.h"

#include <cmath>

#include "item_catalog.h"
#include "inventory.h"
#include "merchant.h"

namespace game {

namespace {

const Item* findItem(const Inventory& inventory, uint32_t formID) {
    for (const InventorySlot& slot : inventory.getAllSlots()) {
        if (!slot.isEmpty() && slot.item.itemId == formID) return &slot.item;
    }
    return nullptr;
}

}  // namespace

int32_t tradeGold(float unitPrice, int32_t count) {
    if (unitPrice <= 0.0f || count <= 0) return 0;
    const double total = static_cast<double>(unitPrice) * static_cast<double>(count);
    const long rounded = std::lround(total);
    if (rounded < 1) return 1;
    return static_cast<int32_t>(rounded);
}

BarterResult buyFromMerchant(Inventory& inventory, Merchant& merchant,
                             const std::string& itemId, int32_t count,
                             float playerMercantile) {
    BarterResult result;
    if (count <= 0 || itemId.empty()) {
        result.reason = "Nothing is selected to buy";
        return result;
    }

    const MerchantItem* stock = merchant.findItem(itemId);
    if (stock == nullptr) {
        result.reason = "The merchant does not stock that";
        return result;
    }
    if (stock->quantity < count) {
        result.reason = "The merchant does not have that many";
        return result;
    }

    const int32_t cost = tradeGold(merchant.calculateBuyPrice(itemId, playerMercantile), count);
    if (cost <= 0) {
        result.reason = "That item is not for sale";
        return result;
    }
    if (playerGold(inventory) < cost) {
        result.reason = "You cannot afford that";
        return result;
    }

    ItemInfo info;
    info.formID = formIDFromItemId(itemId);
    info.name = stock->itemName;
    info.type = stock->category;
    info.weight = stock->weight;
    info.value = stock->baseValue;
    info.resolved = true;

    // Goods first: a full grid or the weight limit must not cost the player the
    // coins before the goods are actually in the pack.
    if (!addPlayerItem(inventory, info, count)) {
        result.reason = "You cannot carry that much";
        return result;
    }
    if (!removePlayerItem(inventory, kGoldFormID, cost)) {
        removePlayerItem(inventory, info.formID, count);
        result.reason = "Your gold could not be paid";
        return result;
    }

    merchant.removeItem(itemId, count);
    merchant.addGold(static_cast<float>(cost));

    result.ok = true;
    result.gold = cost;
    return result;
}

BarterResult sellToMerchant(Inventory& inventory, Merchant& merchant,
                            uint32_t itemFormID, int32_t count,
                            float playerMercantile) {
    BarterResult result;
    if (count <= 0 || itemFormID == 0) {
        result.reason = "Nothing is selected to sell";
        return result;
    }
    // Gold is the currency, so it can never be sold back to the shop.
    if (itemFormID == kGoldFormID) {
        result.reason = "Gold is the currency and cannot be sold";
        return result;
    }
    if (countPlayerItem(inventory, itemFormID) < count) {
        result.reason = "You are not carrying that many";
        return result;
    }

    const Item* sold = findItem(inventory, itemFormID);
    if (sold == nullptr) {
        result.reason = "That item is not in your pack";
        return result;
    }

    // An item the merchant does not stock yet is priced from the record value of
    // the stack being sold rather than from a shelf row.
    const float unitPrice =
        Merchant::sellPriceForValue(static_cast<float>(sold->value), playerMercantile);
    const int32_t earned = tradeGold(unitPrice, count);
    if (earned <= 0) {
        result.reason = "That item is worth nothing";
        return result;
    }
    if (merchant.getGold() < static_cast<float>(earned)) {
        result.reason = "The merchant cannot afford that";
        return result;
    }

    // Removing the last unit erases the row that owns it, so snapshot the sold
    // item before the inventory is mutated.
    ItemInfo soldInfo;
    soldInfo.formID = itemFormID;
    soldInfo.name = sold->name;
    soldInfo.type = sold->type;
    soldInfo.weight = sold->weight;
    soldInfo.value = static_cast<int32_t>(sold->value);
    soldInfo.resolved = true;

    if (!removePlayerItem(inventory, itemFormID, count)) {
        result.reason = "The goods could not be handed over";
        return result;
    }
    if (!addPlayerItem(inventory, goldItemInfo(), earned)) {
        // Gold is weightless, so only a full grid can refuse it. Give the goods
        // back rather than take them for nothing: the row just freed is free.
        addPlayerItem(inventory, soldInfo, count);
        result.reason = "Your gold could not be collected";
        return result;
    }

    merchant.removeGold(static_cast<float>(earned));
    merchant.addItem(MerchantItem(itemIdFromFormID(itemFormID), soldInfo.name,
                                  static_cast<float>(soldInfo.value), unitPrice,
                                  count, soldInfo.type, std::string(),
                                  soldInfo.weight, soldInfo.value));

    result.ok = true;
    result.gold = earned;
    return result;
}

}  // namespace game