#include "merchant.h"
#include <algorithm>
#include <cmath>

Merchant::Merchant(uint32_t npcId, const std::string& shopName,
                   const std::string& shopType)
    : npcId_(npcId), shopName_(shopName), shopType_(shopType), shopGold_(5000.0f) {}

void Merchant::addItem(const MerchantItem& item) {
    // 同じitemIdが既に存在するかチェック
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
                          [&item](const MerchantItem& existing) {
                              return existing.itemId == item.itemId;
                          });

    if (it != inventory_.end()) {
        // 既存アイテムの数量を増やす
        it->quantity += item.quantity;
    } else {
        // 新規アイテムを追加
        inventory_.push_back(item);
    }
}

void Merchant::removeItem(const std::string& itemId) {
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
                          [&itemId](const MerchantItem& item) {
                              return item.itemId == itemId;
                          });

    if (it != inventory_.end()) {
        inventory_.erase(it);
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

    // Charisma ボーナス（最大 20% 割引）
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

    // Charisma ボーナス（最大 20% 増加）
    float charismaBonus = std::clamp(playerCharisma / 100.0f, 0.0f, 0.2f);
    float priceMultiplier = 1.0f + charismaBonus;

    return item->sellPrice * priceMultiplier;
}
