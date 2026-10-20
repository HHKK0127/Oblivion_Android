#pragma once

// Fatigue system tests.
//
// Covers game/fatigue_system.h, the Android port of the original Fatigue AV
// (0x0A) formulas recovered in RE_EXE_REPORT.md §50.2 / §56.1 / §57.3. The
// cases pin the GMST default constants and the exact curve/cost shapes the RE
// pass measured, including the Combat_CalcMeleeDamage "exactly full fatigue
// contributes 1.0" quirk.

#include <string>
#include <vector>

struct FatigueSystemTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class FatigueSystemTests {
public:
    bool runAllTests();
    const std::vector<FatigueSystemTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<FatigueSystemTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
    static bool near(float a, float b, float eps = 1e-4f);
};
