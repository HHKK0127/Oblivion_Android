#pragma once

// MSVC LCG / round half-up tests.
//
// Covers util/msvc_lcg.h and util/numeric.h, the j6-rng-rounding port of
// docs/RE_EXE_REPORT.md section 60.3 (MSVC rand() LCG, Rand() at 0x0047DF80,
// FUN_0047FB70 probability helper with <=) plus the round_half_up helper
// used by the UI call sites.

#include <string>
#include <vector>

struct MsvcLcgTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class MsvcLcgTests {
public:
    bool runAllTests();
    const std::vector<MsvcLcgTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MsvcLcgTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
};
