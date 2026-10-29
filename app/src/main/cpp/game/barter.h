#pragma once

#include <cstdint>
#include <string>

class Merchant;
class Inventory;

namespace game {

// Coins that change hands for count units at unitPrice. Prices are floats but
// gold is an item stack, so a trade pays whole coins and any price at all
// becomes at least one coin. Returns 0 when the trade has no price, which is
// the only case the trade screen and the trade path both treat as not for sale:
// they share this function so a row cannot look affordable and then do nothing.
int32_t tradeGold(float unitPrice, int32_t count);

// Outcome of one barter operation. A failed trade changes nothing, so callers
// can render the previous numbers again without rolling anything back.
struct BarterResult {
    bool ok = false;
    int32_t gold = 0;   // Gold that changed hands, 0 when ok is false
    // Why the trade was refused, ready to show on the trade screen. Empty when
    // ok is true, so the screen never has to guess a reason on its own.
    std::string reason;
};

// Move count units of the merchant stock row itemId into the player inventory,
// paid from the player Gold stack. Fails when the merchant does not stock the
// row, the stock is short, the row has no price, the player cannot pay, or the
// player cannot carry the goods.
BarterResult buyFromMerchant(Inventory& inventory, Merchant& merchant,
                             const std::string& itemId, int32_t count,
                             float playerMercantile = 0.0f);

// Move count units of the base item itemFormID from the player inventory to the
// merchant, paid from the merchant purse. Fails when the player does not hold
// that many, when the row is Gold (never tradable), when the item has no price,
// or when the merchant cannot pay.
BarterResult sellToMerchant(Inventory& inventory, Merchant& merchant,
                            uint32_t itemFormID, int32_t count,
                            float playerMercantile = 0.0f);

}  // namespace game