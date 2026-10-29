#pragma once

// Merchant persistence tests (MERC save section).
//
// A shopkeeper's purse and stock live in memory, so without a save section a
// reload reset every purse to its plugin default and restocked everything the
// player had already bought. SaveManager writes a MERC section next to CNTR and,
// because merchants are materialised lazily from the NPC record the first time
// the player opens their shop, buffers the read state until
// applyPendingMerchantState() hands it to the freshly built merchant.
//
// These tests pin that contract - the materialised round trip, the replacement
// (not merge) of the plugin stock, the carry-forward case where a save happens
// before the player ever traded, and v4 backward compatibility.

#include <string>
#include <vector>

struct MerchantPersistenceTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class MerchantPersistenceTests {
public:
    bool runAllTests();
    const std::vector<MerchantPersistenceTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MerchantPersistenceTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
