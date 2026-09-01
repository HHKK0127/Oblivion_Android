#pragma once

#include <string>
#include <vector>
#include <memory>
#include "inventory.h"

/**
 * @brief Merchant item entry
 * Merchant item information for NPC buying/selling
 */
struct MerchantItem {
    std::string itemId;
    std::string itemName;
    float buyPrice;      // Price for player to buy
    float sellPrice;     // Price for player to sell
    int quantity;        // Stock quantity
    ItemType category;   // Using ItemType instead of ItemCategory
    std::string description;

    MerchantItem(const std::string& id, const std::string& name,
                 float buy, float sell, int qty,
                 ItemType cat, const std::string& desc = "")
        : itemId(id), itemName(name), buyPrice(buy), sellPrice(sell),
          quantity(qty), category(cat), description(desc) {}
};

/**
 * @brief Merchant information
 * NPC shopkeeper's product list and trading rules
 */
class Merchant {
public:
    Merchant(uint32_t npcId, const std::string& shopName,
             const std::string& shopType = "General Goods");
    ~Merchant() = default;

    // Product management
    void addItem(const MerchantItem& item);
    void removeItem(const std::string& itemId, int32_t quantity = 1);
    MerchantItem* findItem(const std::string& itemId);
    const std::vector<MerchantItem>& getInventory() const { return inventory_; }

    // Trade price calculation (considering merchant/player charisma, etc.)
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
    float shopGold_;              // Merchant's cash
};
