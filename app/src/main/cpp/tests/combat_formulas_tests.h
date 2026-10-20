#pragma once

// Combat formula tests.
//
// Covers game/combat_formulas.h, the confirmed FUN_005FEBF0 fragments ported
// from RE_EXE_REPORT.md §56.1 and tmp/re/s57_combat_full.md §3.1 / §13. The
// block cases pin the exact expectations the RE pass wrote down for the port:
// Block 50 / Luck 50 / x 1.0 -> block amount 0.625, weapon block 0.3125,
// hand-to-hand block 0.15625.

#include <string>
#include <vector>

struct CombatFormulasTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class CombatFormulasTests {
public:
    bool runAllTests();
    const std::vector<CombatFormulasTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<CombatFormulasTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
    static bool near(float a, float b, float eps = 1e-4f);
};
