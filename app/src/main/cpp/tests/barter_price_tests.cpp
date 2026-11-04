#include "barter_price_tests.h"

#include "../game/barter.h"
#include "../game/merchant.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

double nowMs() {
    static const auto start = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

// Coefficients are single-precision results of sqrt/trunc arithmetic; the
// expected constants were derived in double precision, so compare with a
// tolerance. Integer-valued prices (ceil outputs) are compared exactly.
bool nearlyEqual(float a, float b) {
    return std::fabs(a - b) < 0.002f;
}

std::string toString(float value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

} // namespace

bool BarterPriceTests::runAllTests() {
    results.clear();
    std::cout << "[BarterPriceTests] Running barter pricing known-value tests..."
              << std::endl;

    test_effective_skill_clamps();
    test_buy_coefficient_known_values();
    test_sell_coefficient_known_values();
    test_disposition_term_truncates_toward_zero();
    test_round_price_ceils();
    test_stolen_discount_and_sentinel();
    test_haggle_acceptance();
    test_buy_price_defaults();
    test_buy_price_haggle_floor();
    test_sell_price_defaults();
    test_sell_price_haggle_cap_and_skill_clamp();
    test_buy_sell_spread();

    const int pass = getPassCount();
    const int fail = getFailCount();
    std::cout << "[BarterPriceTests] Completed: " << pass << " passed, " << fail
              << " failed" << std::endl;
    return fail == 0;
}

int BarterPriceTests::getPassCount() const {
    int count = 0;
    for (const auto& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int BarterPriceTests::getFailCount() const {
    int count = 0;
    for (const auto& result : results) {
        if (!result.passed) {
            ++count;
        }
    }
    return count;
}

std::string BarterPriceTests::getSummary() const {
    std::ostringstream out;
    out << "Barter Price Test Results\n";
    out << "Total: " << results.size() << " | Pass: " << getPassCount()
        << " | Fail: " << getFailCount();
    return out.str();
}

void BarterPriceTests::record(const std::string& name, bool passed,
                              const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
    std::cout << "[BarterPriceTests] " << (passed ? "PASS" : "FAIL") << " " << name;
    if (!msg.empty()) {
        std::cout << " - " << msg;
    }
    std::cout << " (" << ms << " ms)" << std::endl;
}

// FUN_00547B90: clamp(skill + 0.4 * Luck - 20, 0, 100).
void BarterPriceTests::test_effective_skill_clamps() {
    const double t0 = nowMs();
    bool ok = nearlyEqual(game::barterEffectiveSkill(0.0f, 0.0f), 0.0f) &&
              nearlyEqual(game::barterEffectiveSkill(0.0f, 50.0f), 0.0f) &&
              nearlyEqual(game::barterEffectiveSkill(50.0f, 50.0f), 50.0f) &&
              nearlyEqual(game::barterEffectiveSkill(100.0f, 100.0f), 100.0f);
    std::string msg;
    if (!ok) {
        msg = "expected (0,0)->0 (0,50)->0 (50,50)->50 (100,100)->100, got " +
              toString(game::barterEffectiveSkill(0.0f, 0.0f)) + "/" +
              toString(game::barterEffectiveSkill(0.0f, 50.0f)) + "/" +
              toString(game::barterEffectiveSkill(50.0f, 50.0f)) + "/" +
              toString(game::barterEffectiveSkill(100.0f, 100.0f));
    }
    record("effective_skill_clamps", ok, msg, static_cast<float>(nowMs() - t0));
}

// Buy coefficient 0.01 * (300 - 9.9 * sqrt(200 - min(x, 100))
//                          - 1.5 * trunc((y - 50) / 10)).
void BarterPriceTests::test_buy_coefficient_known_values() {
    const double t0 = nowMs();
    const float a = game::barterBuyCoefficient(0.0f, 50);
    const float b = game::barterBuyCoefficient(100.0f, 50);
    const float c = game::barterBuyCoefficient(0.0f, 0);
    bool ok = nearlyEqual(a, 1.5999286f) && nearlyEqual(b, 2.01f) &&
              nearlyEqual(c, 1.6749286f);
    std::string msg;
    if (!ok) {
        msg = "expected 1.5999286/2.01/1.6749286, got " + toString(a) + "/" +
              toString(b) + "/" + toString(c);
    }
    record("buy_coefficient_known_values", ok, msg, static_cast<float>(nowMs() - t0));
}

// Sell coefficient 0.01 * (5 * sqrt(200 - min(x, 100)) - 10
//                          + 1.5 * trunc((y - 50) / 10)).
void BarterPriceTests::test_sell_coefficient_known_values() {
    const double t0 = nowMs();
    const float a = game::barterSellCoefficient(0.0f, 50);
    const float b = game::barterSellCoefficient(100.0f, 50);
    const float c = game::barterSellCoefficient(0.0f, 0);
    bool ok = nearlyEqual(a, 0.60710678f) && nearlyEqual(b, 0.40f) &&
              nearlyEqual(c, 0.53210678f);
    std::string msg;
    if (!ok) {
        msg = "expected 0.60710678/0.40/0.53210678, got " + toString(a) + "/" +
              toString(b) + "/" + toString(c);
    }
    record("sell_coefficient_known_values", ok, msg, static_cast<float>(nowMs() - t0));
}

// The disposition term divides by 10 in integer arithmetic, so it truncates
// toward zero: y = 95 gives +4 and y = 5 gives -4 (floor would give -5).
void BarterPriceTests::test_disposition_term_truncates_toward_zero() {
    const double t0 = nowMs();
    const float high = game::barterBuyCoefficient(0.0f, 95);
    const float low = game::barterBuyCoefficient(0.0f, 5);
    bool ok = nearlyEqual(high, 1.5399286f) && nearlyEqual(low, 1.6599286f);
    std::string msg;
    if (!ok) {
        msg = "expected 1.5399286/1.6599286, got " + toString(high) + "/" +
              toString(low);
    }
    record("disposition_term_truncates_toward_zero", ok, msg,
           static_cast<float>(nowMs() - t0));
}

// The trade path rounds with ceilf in both directions (0x484370).
void BarterPriceTests::test_round_price_ceils() {
    const double t0 = nowMs();
    bool ok = game::barterRoundPrice(1.2f) == 2.0f &&
              game::barterRoundPrice(151.7767f) == 152.0f &&
              game::barterRoundPrice(4.0f) == 4.0f;
    std::string msg;
    if (!ok) {
        msg = "expected 2/152/4, got " + toString(game::barterRoundPrice(1.2f)) +
              "/" + toString(game::barterRoundPrice(151.7767f)) + "/" +
              toString(game::barterRoundPrice(4.0f));
    }
    record("round_price_ceils", ok, msg, static_cast<float>(nowMs() - t0));
}

// FUN_00484850(item, 1) returns -1.0 when the goods are not stolen; any other
// percent scales the raw price before rounding.
void BarterPriceTests::test_stolen_discount_and_sentinel() {
    const double t0 = nowMs();
    bool ok = game::barterApplyStolenDiscount(100.0f, -1.0f) == 100.0f &&
              game::barterApplyStolenDiscount(100.0f, 50.0f) == 50.0f &&
              game::barterApplyStolenDiscount(100.0f, 100.0f) == 100.0f;
    std::string msg;
    if (!ok) {
        msg = "expected 100/50/100 for percents -1/50/100";
    }
    record("stolen_discount_and_sentinel", ok, msg, static_cast<float>(nowMs() - t0));
}

// FUN_00548130: slider 0 always passes; otherwise
// chance = (int(merchant) - int(player) + 100) / 10 - 0.55 * slider
//          + 0.5 * trunc((disposition - 10) / 10) must be >= 0.
void BarterPriceTests::test_haggle_acceptance() {
    const double t0 = nowMs();
    bool ok = game::barterHaggleAccepts(0.0f, 0.0f, 50, 0.0f) &&
              game::barterHaggleAccepts(0.0f, 0.0f, 50, 20.0f) &&
              !game::barterHaggleAccepts(0.0f, 0.0f, 50, 30.0f);
    std::string msg;
    if (!ok) {
        msg = "slider 0 and 20 should pass at neutral skills, 30 should fail";
    }
    record("haggle_acceptance", ok, msg, static_cast<float>(nowMs() - t0));
}

// Buy price = max(ceil(buyCoefficient * value), value) at vanilla defaults
// (no merchant barter skill, neutral disposition, no haggle).
void BarterPriceTests::test_buy_price_defaults() {
    const double t0 = nowMs();
    const float gem = Merchant::buyPriceForValue(250.0f);
    const float apple = Merchant::buyPriceForValue(5.0f);
    const float cheap = Merchant::buyPriceForValue(1.0f);
    bool ok = gem == 400.0f && apple == 8.0f && cheap == 2.0f;
    std::string msg;
    if (!ok) {
        msg = "expected 400/8/2, got " + toString(gem) + "/" + toString(apple) +
              "/" + toString(cheap);
    }
    record("buy_price_defaults", ok, msg, static_cast<float>(nowMs() - t0));
}

// Haggling lowers the raw buy price, but the base-value floor keeps the
// player from buying below the item's value.
void BarterPriceTests::test_buy_price_haggle_floor() {
    const double t0 = nowMs();
    const float moderate = Merchant::buyPriceForValue(250.0f, 0.0f, 50, 100.0f);
    const float extreme = Merchant::buyPriceForValue(250.0f, 0.0f, 50, 500.0f);
    bool ok = moderate == 250.0f && extreme == 250.0f;
    std::string msg;
    if (!ok) {
        msg = "expected 250/250, got " + toString(moderate) + "/" + toString(extreme);
    }
    record("buy_price_haggle_floor", ok, msg, static_cast<float>(nowMs() - t0));
}

// Sell price = min(ceil(stolen-adjusted sellCoefficient * value), value) at
// vanilla defaults. ceil(0.6071 * 1) leaves one coin for a value-1 item.
void BarterPriceTests::test_sell_price_defaults() {
    const double t0 = nowMs();
    const float gem = Merchant::sellPriceForValue(250.0f);
    const float apple = Merchant::sellPriceForValue(5.0f);
    const float cheap = Merchant::sellPriceForValue(1.0f);
    bool ok = gem == 152.0f && apple == 4.0f && cheap == 1.0f;
    std::string msg;
    if (!ok) {
        msg = "expected 152/4/1, got " + toString(gem) + "/" + toString(apple) +
              "/" + toString(cheap);
    }
    record("sell_price_defaults", ok, msg, static_cast<float>(nowMs() - t0));
}

// Haggling raises the raw sell price, but the base-value cap keeps the payout
// at or below the item's value. The merchant effective skill clamps at 100.
void BarterPriceTests::test_sell_price_haggle_cap_and_skill_clamp() {
    const double t0 = nowMs();
    const float haggled = Merchant::sellPriceForValue(250.0f, 0.0f, 50, 100.0f);
    const float skilled = Merchant::sellPriceForValue(250.0f, 100.0f);
    const float overSkilled = Merchant::sellPriceForValue(250.0f, 900.0f);
    bool ok = haggled == 250.0f && skilled == 100.0f && overSkilled == 100.0f;
    std::string msg;
    if (!ok) {
        msg = "expected 250/100/100, got " + toString(haggled) + "/" +
              toString(skilled) + "/" + toString(overSkilled);
    }
    record("sell_price_haggle_cap_and_skill_clamp", ok, msg,
           static_cast<float>(nowMs() - t0));
}

// A merchant always buys high and sells low: the buy price for a value sits
// above the sell price for the same value.
void BarterPriceTests::test_buy_sell_spread() {
    const double t0 = nowMs();
    const float buy = Merchant::buyPriceForValue(250.0f);
    const float sell = Merchant::sellPriceForValue(250.0f);
    bool ok = buy == 400.0f && sell == 152.0f && buy > sell;
    std::string msg;
    if (!ok) {
        msg = "expected buy 400 > sell 152, got " + toString(buy) + "/" +
              toString(sell);
    }
    record("buy_sell_spread", ok, msg, static_cast<float>(nowMs() - t0));
}
