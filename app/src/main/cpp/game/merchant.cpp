#include "merchant.h"
#include "barter.h"
#include <algorithm>
#include <cmath>

Merchant::Merchant(uint32_t npcId, const std::string& shopName,
                   const std::string& shopType)
    : npcId_(npcId), shopName_(shopName), shopType_(shopType), shopGold_(5000.0f) {}

void Merchant::addItem(const MerchantItem& item) {
    // Check if same itemId already exists
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
                          [&item](const MerchantItem& existing) {
                              return existing.itemId == item.itemId;
                          });

    if (it != inventory_.end()) {
        // Increase quantity of existing item
        it->quantity += item.quantity;
    } else {
        // Add new item
        inventory_.push_back(item);
    }
}

void Merchant::removeItem(const std::string& itemId, int32_t quantity) {
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
                          [&itemId](const MerchantItem& item) {
                              return item.itemId == itemId;
                          });

    if (it != inventory_.end()) {
        it->quantity -= quantity;
        // Only remove entry when quantity reaches zero or below
        if (it->quantity <= 0) {
            inventory_.erase(it);
        }
    }
}

MerchantItem* Merchant::findItem(const std::string& itemId) {
    return const_cast<MerchantItem*>(static_cast<const Merchant*>(this)->findItem(itemId));
}

const MerchantItem* Merchant::findItem(const std::string& itemId) const {
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
                          [&itemId](const MerchantItem& item) {
                              return item.itemId == itemId;
                          });

    return (it != inventory_.end()) ? &(*it) : nullptr;
}

float Merchant::effectiveBarterSkill() const {
    return game::barterEffectiveSkill(barterSkill_, barterLuck_);
}

float Merchant::buyPriceForValue(float baseValue, float merchantEffectiveSkill,
                                 int32_t disposition, float haggleOffset) {
    if (baseValue <= 0.0f) {
        return 0.0f;
    }

    // Price path FUN_00488e50 buy branch: the merchant coefficient is lowered
    // by the haggle slider, the product is rounded up (ceilf), and the base
    // value acts as the floor the player can never be charged less than.
    const float coef = game::barterBuyCoefficient(merchantEffectiveSkill, disposition) -
                       game::kBarterHaggleScale * haggleOffset;
    const float price = game::barterRoundPrice(coef * baseValue);
    return std::max(price, baseValue);
}

float Merchant::sellPriceForValue(float baseValue, float merchantEffectiveSkill,
                                  int32_t disposition, float haggleOffset,
                                  float stolenPercent) {
    if (baseValue <= 0.0f) {
        return 0.0f;
    }

    // FUN_00488e50 sell branch: multiply, then apply the stolen-goods discount
    // as a raw float (the percent already carries its own floor), then ceil,
    // then cap at the base value saved before the multiply.
    const float coef = game::barterSellCoefficient(merchantEffectiveSkill, disposition) +
                       game::kBarterHaggleScale * haggleOffset;
    float raw = coef * baseValue;
    raw = game::barterApplyStolenDiscount(raw, stolenPercent);
    const float price = game::barterRoundPrice(raw);
    return std::min(price, baseValue);
}

float Merchant::calculateBuyPrice(const std::string& itemId, float haggleOffset) const {
    const MerchantItem* item = findItem(itemId);

    if (item == nullptr) {
        return 0.0f;
    }

    const float value = item->baseValue > 0 ? static_cast<float>(item->baseValue) : item->buyPrice;
    return buyPriceForValue(value, effectiveBarterSkill(), disposition_, haggleOffset);
}

float Merchant::calculateSellPrice(const std::string& itemId, float haggleOffset,
                                   float stolenPercent) const {
    const MerchantItem* item = findItem(itemId);

    if (item == nullptr) {
        return 0.0f;
    }

    const float value = item->baseValue > 0 ? static_cast<float>(item->baseValue) : item->sellPrice;
    return sellPriceForValue(value, effectiveBarterSkill(), disposition_, haggleOffset, stolenPercent);
}
