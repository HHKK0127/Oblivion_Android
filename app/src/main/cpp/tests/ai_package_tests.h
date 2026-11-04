#pragma once

// ============================================================================
// Host-side unit tests for game/ai_package — canonical package selection.
// ============================================================================
// Asserts the RE-canon first-match chain (PackageSelect_Main, 0x569020):
//   flag predicates -> schedule (day mask + [start, start+duration) window)
//   -> CTDA condition list (game::evaluate_list bridge).
// Also pins the documented port deviations: no packages[0] fallback, and
// skip/unsupported CTDA results are accepted while the runtime bridge is
// being built. Conventions mirror ConditionTests.
// ============================================================================

#include <cstdint>
#include <string>
#include <vector>

struct AiPackageTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class AiPackageTests {
public:
    bool runAllTests();

    const std::vector<AiPackageTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void test_first_match_scan_order();
    void test_no_eligible_package_returns_nullptr();
    void test_schedule_window_midnight_wrap();
    void test_schedule_day_mask();
    void test_schedule_zero_duration_never_matches();
    void test_disabled_schedule_always_matches();
    void test_ctda_and_pass_and_fail();
    void test_ctda_skip_is_accepted();
    void test_ctda_empty_list_passes();
    void test_ctda_without_runtime_passes();
    void test_chain_blocks_downstream_stage();

    void record(const std::string& name, bool passed,
                const std::string& msg = "", float ms = 0.0f);

    std::vector<AiPackageTestResult> results;
};
