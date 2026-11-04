#pragma once

// =============================================================================
// BarterPriceTests — known-value coverage for the reverse-engineered barter
// pricing model (Oblivion.exe FUN_00488e50 price path, FUN_00548130 haggle
// acceptance, FUN_00547B90 effective skill; see docs/RE_EXE_REPORT.md §57.5
// and §59.5). The pure functions under test are game::barter* helpers and the
// value-based Merchant::buyPriceForValue / Merchant::sellPriceForValue statics.
// Deviations from canon (player-Barter > 3 overrides, merchant-gold capping)
// are documented in the implementation and are not asserted here.
// Conventions mirror AiPackageTests.
// =============================================================================

#include <cstdint>
#include <string>
#include <vector>

struct BarterPriceTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class BarterPriceTests {
public:
    bool runAllTests();

    const std::vector<BarterPriceTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void test_effective_skill_clamps();
    void test_buy_coefficient_known_values();
    void test_sell_coefficient_known_values();
    void test_disposition_term_truncates_toward_zero();
    void test_round_price_ceils();
    void test_stolen_discount_and_sentinel();
    void test_haggle_acceptance();
    void test_buy_price_defaults();
    void test_buy_price_haggle_floor();
    void test_sell_price_defaults();
    void test_sell_price_haggle_cap_and_skill_clamp();
    void test_buy_sell_spread();

    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);

    std::vector<BarterPriceTestResult> results;
};
