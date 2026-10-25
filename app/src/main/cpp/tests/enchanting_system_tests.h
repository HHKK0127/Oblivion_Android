#pragma once

// Phase 65: regression tests for the Oblivion magic effect (MGEF) tables in
// EnchantingSystem.
//
// The runtime tables were rewritten from reverse engineered data: the effect
// enum now mirrors the 88 effect families the shipped records actually use, and
// magicEffectFromCode() maps the 145 measured MGEF editorID codes onto them.
// Because both tables are positional / hand written, a wrong entry is silent -
// a shifted name table renames every effect after it, and a missing code turns a
// working enchantment into UNKNOWN. The expectations below are generated from
// the measured data (tmp/re/v35_mgef_code_table.csv plus its assignment
// generator) rather than copied from the implementation, so they fail when the
// implementation drifts.

#include <cstdint>
#include <string>
#include <vector>

struct EnchantingSystemTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class EnchantingSystemTests {
public:
    // Returns true if all tests pass.
    bool runAllTests();

    // Valid after runAllTests().
    const std::vector<EnchantingSystemTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void testEnumShape();
    void testEffectCodeMapping();
    void testRejectedEffectCodes();
    void testMalformedEffectCodes();
    void testEffectNameTable();
    void testEnchantmentDefaults();
    void testCreateEnchantment();
    void testEffectMagnitude();
    void testEffectDuration();
    void testEnchantCost();
    void testEnchantmentsFromEsm();

    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);

    std::vector<EnchantingSystemTestResult> results;
};
