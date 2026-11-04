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

// Barter price model reverse engineered from Oblivion.exe (see
// docs/RE_EXE_REPORT.md §57.5 / §59.5 and the price path FUN_00488e50).
// Everything below is a pure function so the trade screen, the merchant and
// the tests share one implementation of the original numbers.

// GMST constants read from the executable data segment.
constexpr float kBarterBuyBase = 300.0f;             // fBarterBuyBase
constexpr float kBarterBuyMult = 9.9f;               // fBarterBuyMult
constexpr float kBarterSellBase = -10.0f;            // fBarterSellBase
constexpr float kBarterSellMult = 5.0f;              // fBarterSellMult
constexpr float kBarterDispositionMod = 1.5f;        // fBarterDispositionMod
constexpr float kBarterHaggleScale = 0.01f;          // percents are stored whole
constexpr float kBarterHaggleAcceptBase = 0.55f;     // fBarterHaggleBase
constexpr float kBarterHaggleDispositionMult = 0.5f;

// Effective Mercantile skill of a trader:
// clamp(skill + 0.4 * Luck - 20, 0, 100) (FUN_00547B90).
float barterEffectiveSkill(float skill, float luck);

// Price coefficients keyed by the MERCHANT's effective skill and disposition
// (FUN_005481C0 buy / FUN_00548240 sell). The disposition term divides as
// integers, which truncates toward zero exactly like the original.
float barterBuyCoefficient(float merchantEffectiveSkill, int32_t disposition);
float barterSellCoefficient(float merchantEffectiveSkill, int32_t disposition);

// The trade path rounds unit prices up with ceilf in both directions
// (FUN_00484370 as called from FUN_00488e50).
float barterRoundPrice(float price);

// Haggle acceptance test (FUN_00548130). slider is the barter window slider
// amount and 0 always passes. Both skills arrive as whole numbers in the
// original, so fractional parts are dropped before the difference.
bool barterHaggleAccepts(float playerEffectiveSkill, float merchantEffectiveSkill,
                         int32_t disposition, float slider);

// Stolen-goods correction of the sell branch. percent is the stolen value
// percentage in [0, 100]; a negative percent is the "not stolen" sentinel of
// FUN_00484850 and leaves the price untouched. The correction multiplies the
// raw price before it is rounded up.
float barterApplyStolenDiscount(float price, float percent);

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
// player cannot carry the goods. haggleOffset is the player's haggle amount in
// percent ([player+0x11C] in the original), which lowers the buy price.
BarterResult buyFromMerchant(Inventory& inventory, Merchant& merchant,
                             const std::string& itemId, int32_t count,
                             float haggleOffset = 0.0f);

// Move count units of the base item itemFormID from the player inventory to the
// merchant, paid from the merchant purse. Fails when the player does not hold
// that many, when the row is Gold (never tradable), when the item has no price,
// or when the merchant cannot pay. haggleOffset raises the sell price by its
// percent, and stolenPercent applies the stolen-goods correction (-1.0 when the
// goods are not stolen).
BarterResult sellToMerchant(Inventory& inventory, Merchant& merchant,
                            uint32_t itemFormID, int32_t count,
                            float haggleOffset = 0.0f,
                            float stolenPercent = -1.0f);

}  // namespace game