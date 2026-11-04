#pragma once

// Host-side unit tests for game/condition_system (CTDA condition evaluator).
// Semantics are asserted against the verbatim RE port of the shipping
// evaluator chain (FUN_0056a510 / FUN_0056ac50 / FUN_0056a3b0); see
// docs/RE_EXE_REPORT.md section 58. Conventions mirror ObjectLodTests.

#include <cstdint>
#include <string>
#include <vector>

struct ConditionTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class ConditionTests {
public:
    bool runAllTests();

    const std::vector<ConditionTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void test_parse_layout();
    void test_compare_ops();
    void test_eval_pass_and_fail();
    void test_eval_query_failure_skip();
    void test_eval_unknown_function_skip();
    void test_eval_swap_operand();
    void test_eval_null_param_fallback();
    void test_eval_needs_ref_guard();
    void test_eval_global_compare();
    void test_eval_disposition_skip_marker();
    void test_list_empty_and_null();
    void test_list_and_short_circuit();
    void test_list_or_group_close_pass();
    void test_list_or_group_close_fail();
    void test_list_or_group_all_skip();
    void test_list_or_group_skip_then_pass();
    void test_list_secondary_adoption();
    void test_list_context_override();
    void test_table_metadata();

    void record(const std::string& name, bool passed,
                const std::string& msg = "", float ms = 0.0f);

    std::vector<ConditionTestResult> results;
};
