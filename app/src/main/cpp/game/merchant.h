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

    // Trade price calculation using the barter model reverse engineered from
    // Oblivion.exe (docs/RE_EXE_REPORT.md §57.5 / §59.5, price path
    // FUN_00488e50). haggleOffset is the player's haggle-slider percent
    // (0..100, whole numbers; it lowers buy prices and raises sell prices).
    // stolenPercent is the stolen-goods value percent from
    // FUN_00484850(item, 1); the -1.0 default means the goods are not stolen.
    float calculateBuyPrice(const std::string& itemId, float haggleOffset = 0.0f) const;
    float calculateSellPrice(const std::string& itemId, float haggleOffset = 0.0f,
                             float stolenPercent = -1.0f) const;

    // Value-based variants, used for items the merchant does not stock yet
    // (every player sale). Defaults describe a merchant with no barter skill,
    // neutral disposition and no haggle, matching the vanilla floor case.
    static float buyPriceForValue(float baseValue, float merchantEffectiveSkill = 0.0f,
                                  int32_t disposition = 50, float haggleOffset = 0.0f);
    static float sellPriceForValue(float baseValue, float merchantEffectiveSkill = 0.0f,
                                   int32_t disposition = 50, float haggleOffset = 0.0f,
                                   float stolenPercent = -1.0f);

    // Getters
    uint32_t getNpcId() const { return npcId_; }
    const std::string& getShopName() const { return shopName_; }
    const std::string& getShopType() const { return shopType_; }
    float getGold() const { return shopGold_; }
    void setGold(float amount) { shopGold_ = std::max(0.0f, amount); }
    void addGold(float amount) { shopGold_ += amount; }
    void removeGold(float amount) { shopGold_ = std::max(0.0f, shopGold_ - amount); }

    // Barter inputs. The buy/sell coefficients key off the merchant's
    // Barter-skill actor value (confirmed in the price path), and disposition
    // enters through fBarterDispositionMod. The defaults mirror a vanilla
    // commoner: no barter training and a neutral attitude.
    float getBarterSkill() const { return barterSkill_; }
    float getBarterLuck() const { return barterLuck_; }
    int32_t getDisposition() const { return disposition_; }
    void setBarterSkill(float skill) { barterSkill_ = skill; }
    void setBarterLuck(float luck) { barterLuck_ = luck; }
    void setDisposition(int32_t disposition) { disposition_ = disposition; }

    // clamp(skill + 0.4 * Luck - 20, 0, 100), from FUN_00547B90.
    float effectiveBarterSkill() const;

private:
    uint32_t npcId_;
    std::string shopName_;
    std::string shopType_;        // "General Goods", "Weapons", "Magic", etc.
    std::vector<MerchantItem> inventory_;
    float shopGold_;              // Merchant's cash
    float barterSkill_ = 0.0f;    // Merchant Barter actor value
    float barterLuck_ = 50.0f;    // Merchant Luck actor value
    int32_t disposition_ = 50;    // Merchant disposition toward the player
};
