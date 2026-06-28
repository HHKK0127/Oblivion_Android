#pragma once

#include <string>
#include <vector>
#include <memory>
#include "inventory.h"

/**
 * @brief マーチャント商品エントリ
 * NPCが売買する商品情報
 */
struct MerchantItem {
    std::string itemId;
    std::string itemName;
    float buyPrice;      // プレイヤーが買う価格
    float sellPrice;     // プレイヤーが売る価格
    int quantity;        // 在庫数
    ItemType category;   // Using ItemType instead of ItemCategory
    std::string description;

    MerchantItem(const std::string& id, const std::string& name,
                 float buy, float sell, int qty,
                 ItemType cat, const std::string& desc = "")
        : itemId(id), itemName(name), buyPrice(buy), sellPrice(sell),
          quantity(qty), category(cat), description(desc) {}
};

/**
 * @brief マーチャント情報
 * NPC店主の商品リストと取引ルール
 */
class Merchant {
public:
    Merchant(uint32_t npcId, const std::string& shopName,
             const std::string& shopType = "General Goods");
    ~Merchant() = default;

    // 商品管理
    void addItem(const MerchantItem& item);
    void removeItem(const std::string& itemId);
    MerchantItem* findItem(const std::string& itemId);
    const std::vector<MerchantItem>& getInventory() const { return inventory_; }

    // 取引価格計算（マーチャント/プレイヤーの説得度などを考慮）
    float calculateBuyPrice(const std::string& itemId, float playerCharisma = 0.0f) const;
    float calculateSellPrice(const std::string& itemId, float playerCharisma = 0.0f) const;

    // Getters
    uint32_t getNpcId() const { return npcId_; }
    const std::string& getShopName() const { return shopName_; }
    const std::string& getShopType() const { return shopType_; }
    float getGold() const { return shopGold_; }
    void addGold(float amount) { shopGold_ += amount; }
    void removeGold(float amount) { shopGold_ = std::max(0.0f, shopGold_ - amount); }

private:
    uint32_t npcId_;
    std::string shopName_;
    std::string shopType_;        // "General Goods", "Weapons", "Magic", etc.
    std::vector<MerchantItem> inventory_;
    float shopGold_;              // マーチャントの現金
};
