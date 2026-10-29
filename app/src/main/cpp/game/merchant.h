#pragma once

#include <algorithm>
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

    // Record data of the stock row, kept next to the trade price so that a
    // purchase can be moved into the player inventory without resolving the
    // ESM record again.
    float weight = 0.0f;
    int32_t baseValue = 0;

    MerchantItem(const std::string& id, const std::string& name,
                 float buy, float sell, int qty,
                 ItemType cat, const std::string& desc = "",
                 float itemWeight = 0.0f, int32_t itemValue = 0)
        : itemId(id), itemName(name), buyPrice(buy), sellPrice(sell),
          quantity(qty), category(cat), description(desc),
          weight(itemWeight), baseValue(itemValue) {}
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
    // Drops every stock row so a restored save can replace the ESM-derived
    // starting stock instead of merging with it.
    void clearStock() { inventory_.clear(); }
    MerchantItem* findItem(const std::string& itemId);
    const MerchantItem* findItem(const std::string& itemId) const;
    const std::vector<MerchantItem>& getInventory() const { return inventory_; }

    // Trade price calculation (considering merchant/player charisma, etc.)
    float calculateBuyPrice(const std::string& itemId, float playerCharisma = 0.0f) const;
    float calculateSellPrice(const std::string& itemId, float playerCharisma = 0.0f) const;

    // Value-based variants, used for items the merchant does not stock yet
    // (every player sale). Provisional: the barter formula of Oblivion.exe is
    // not reverse engineered yet, so buying costs the full value and selling
    // returns kProvisionalSellRatio of it. Replace both once RE lands.
    static constexpr float kProvisionalSellRatio = 0.4f;
    static float buyPriceForValue(float baseValue, float playerCharisma = 0.0f);
    static float sellPriceForValue(float baseValue, float playerCharisma = 0.0f);

    // Getters
    uint32_t getNpcId() const { return npcId_; }
    const std::string& getShopName() const { return shopName_; }
    const std::string& getShopType() const { return shopType_; }
    float getGold() const { return shopGold_; }
    void setGold(float amount) { shopGold_ = std::max(0.0f, amount); }
    void addGold(float amount) { shopGold_ += amount; }
    void removeGold(float amount) { shopGold_ = std::max(0.0f, shopGold_ - amount); }

private:
    uint32_t npcId_;
    std::string shopName_;
    std::string shopType_;        // "General Goods", "Weapons", "Magic", etc.
    std::vector<MerchantItem> inventory_;
    float shopGold_;              // Merchant's cash
};
