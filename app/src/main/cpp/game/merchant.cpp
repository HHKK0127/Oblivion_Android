#include "merchant.h"
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
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
                          [&itemId](MerchantItem& item) {
                              return item.itemId == itemId;
                          });

    return (it != inventory_.end()) ? &(*it) : nullptr;
}

float Merchant::calculateBuyPrice(const std::string& itemId, float playerCharisma) const {
    auto item = std::find_if(inventory_.begin(), inventory_.end(),
                            [&itemId](const MerchantItem& i) {
                                return i.itemId == itemId;
                            });

    if (item == inventory_.end()) {
        return 0.0f;
    }

    // Charisma bonus (max 20% discount)
    float charismaBonus = std::clamp(playerCharisma / 100.0f, 0.0f, 0.2f);
    float priceMultiplier = 1.0f - charismaBonus;

    return item->buyPrice * priceMultiplier;
}

float Merchant::calculateSellPrice(const std::string& itemId, float playerCharisma) const {
    auto item = std::find_if(inventory_.begin(), inventory_.end(),
                            [&itemId](const MerchantItem& i) {
                                return i.itemId == itemId;
                            });

    if (item == inventory_.end()) {
        return 0.0f;
    }

    // Charisma bonus (max 20% increase)
    float charismaBonus = std::clamp(playerCharisma / 100.0f, 0.0f, 0.2f);
    float priceMultiplier = 1.0f + charismaBonus;

    return item->sellPrice * priceMultiplier;
}
