#include "condition_tests.h"

#include "../game/condition_system.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

using game::compare_conditions;
using game::ConditionItem;
using game::ConditionRuntime;
using game::EvalResult;
using game::evaluate_list;
using game::evaluate_single;
using game::find_cond_function;
using game::kCondFunctionCount;
using game::kCondFunctions;

double nowMs() {
    static const auto start = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

uint32_t float_bits(float f) {
    uint32_t u = 0;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

// CTDA flag byte: bit0 OR, bit1 swapped, bit2 uses global, bits5-7 comparison op.
uint8_t make_flag(bool is_or, bool swapped, bool uses_global, uint32_t op) {
    return static_cast<uint8_t>((is_or ? 1u : 0u) | (swapped ? 2u : 0u) |
                                (uses_global ? 4u : 0u) | ((op & 7u) << 5));
}

ConditionItem make_item(uint8_t flag, uint32_t cmp, uint16_t fn,
                        uint32_t p1 = 0, uint32_t p2 = 0) {
    ConditionItem item;
    item.flag = flag;
    item.comparison_value = cmp;
    item.function_index = fn;
    item.param1 = p1;
    item.param2 = p2;
    return item;
}

// Table rows used by the tests (verified against kCondFunctions):
//   1  GetDistance        param_count=1 needs_ref=1
//   12 GetSecondsPassed   param_count=0 needs_ref=0
//   58 GetStage           param_count=1 needs_ref=0
//   76 GetDisposition     param_count=1 needs_ref=1  (0x4C skip-marker fn)
//   2 is absent from the table (find_cond_function(2) == nullptr)
constexpr uint16_t kFnDistance = 1;
constexpr uint16_t kFnSecondsPassed = 12;
constexpr uint16_t kFnStage = 58;
constexpr uint16_t kFnDisposition = 0x4C;
constexpr uint16_t kFnUnknown = 999;

class FakeRuntime : public ConditionRuntime {
public:
    bool query(uint16_t index, uint32_t subject, uint32_t param1, uint32_t param2,
               float& result) override {
        ++query_count;
        last_index = index;
        last_subject = subject;
        last_param1 = param1;
        last_param2 = param2;
        if (query_failures.count(index) != 0) {
            return false;
        }
        const auto it = query_results.find(index);
        result = (it != query_results.end()) ? it->second : 0.0f;
        return true;
    }

    float global_value(uint32_t global_form_id) override {
        ++global_calls;
        const auto it = globals.find(global_form_id);
        return (it != globals.end()) ? it->second : 0.0f;
    }

    uint32_t context_override(uint32_t) const override { return override_value; }

    std::map<uint16_t, float> query_results;
    std::set<uint16_t> query_failures;
    std::map<uint32_t, float> globals;
    uint32_t override_value = 0;

    int query_count = 0;
    int global_calls = 0;
    uint16_t last_index = 0;
    uint32_t last_subject = 0;
    uint32_t last_param1 = 0;
    uint32_t last_param2 = 0;
};

std::string trim_name(const char* padded) {
    return std::string(padded, std::strcspn(padded, " "));
}

}  // namespace

bool ConditionTests::runAllTests() {
    results.clear();

    std::cout << "[ConditionTests] Running condition system tests..." << std::endl;

    test_parse_layout();
    test_compare_ops();
    test_eval_pass_and_fail();
    test_eval_query_failure_skip();
    test_eval_unknown_function_skip();
    test_eval_swap_operand();
    test_eval_null_param_fallback();
    test_eval_needs_ref_guard();
    test_eval_global_compare();
    test_eval_disposition_skip_marker();
    test_list_empty_and_null();
    test_list_and_short_circuit();
    test_list_or_group_close_pass();
    test_list_or_group_close_fail();
    test_list_or_group_all_skip();
    test_list_or_group_skip_then_pass();
    test_list_secondary_adoption();
    test_list_context_override();
    test_table_metadata();

    const int pass = getPassCount();
    const int fail = getFailCount();
    std::cout << "[ConditionTests] Completed: " << pass << " passed, "
              << fail << " failed" << std::endl;

    return fail == 0;
}

int ConditionTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (r.passed) ++count;
    }
    return count;
}

int ConditionTests::getFailCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (!r.passed) ++count;
    }
    return count;
}

std::string ConditionTests::getSummary() const {
    std::ostringstream out;
    out << "Condition System Test Results\n";
    out << "Total: " << results.size()
        << " | Pass: " << getPassCount()
        << " | Fail: " << getFailCount();
    return out.str();
}

void ConditionTests::record(const std::string& name, bool passed,
                            const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
    std::cout << "[ConditionTests] " << (passed ? "PASS" : "FAIL") << " " << name;
    if (!msg.empty()) {
        std::cout << " - " << msg;
    }
    std::cout << " (" << ms << " ms)" << std::endl;
}

void ConditionTests::test_parse_layout() {
    const double t0 = nowMs();
    const std::string name = "ConditionItem::parse layout";

    uint8_t buf[20] = {};
    buf[0] = make_flag(true, true, true, 3);
    const uint32_t cv = float_bits(1.5f);
    std::memcpy(buf + 4, &cv, 4);
    const uint16_t fn = 0x1234;
    std::memcpy(buf + 8, &fn, 2);
    const uint32_t p1 = 0xAAAA;
    std::memcpy(buf + 12, &p1, 4);
    const uint32_t p2 = 0xBBBB;
    std::memcpy(buf + 16, &p2, 4);

    ConditionItem out;
    const bool ok = ConditionItem::parse(buf, sizeof(buf), out) &&
                    out.is_or() && out.is_swapped() && out.uses_global() &&
                    out.comparison_op() == 3 &&
                    out.compare_float() == 1.5f &&
                    out.function_index == 0x1234 &&
                    out.param1 == 0xAAAA && out.param2 == 0xBBBB;
    const bool rejects_short = !ConditionItem::parse(buf, 19, out);
    const bool rejects_null = !ConditionItem::parse(nullptr, 20, out);

    const bool pass = ok && rejects_short && rejects_null;
    record(name, pass, pass ? "" : "field layout or size guard mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_compare_ops() {
    const double t0 = nowMs();
    const std::string name = "compare_conditions ops 0-7";

    bool pass = true;
    pass = pass && compare_conditions(0, 1.0f, 1.0f) == 1;   // Equal
    pass = pass && compare_conditions(0, 1.0f, 2.0f) == 0;
    pass = pass && compare_conditions(1, 1.0f, 2.0f) == 1;   // NotEqual
    pass = pass && compare_conditions(1, 1.0f, 1.0f) == 0;
    pass = pass && compare_conditions(2, 2.0f, 1.0f) == 1;   // Greater
    pass = pass && compare_conditions(2, 1.0f, 1.0f) == 0;
    pass = pass && compare_conditions(3, 1.0f, 1.0f) == 1;   // GreaterEqual
    pass = pass && compare_conditions(3, 0.9f, 1.0f) == 0;
    pass = pass && compare_conditions(4, 1.0f, 2.0f) == 1;   // Less
    pass = pass && compare_conditions(4, 2.0f, 2.0f) == 0;
    pass = pass && compare_conditions(5, 2.0f, 2.0f) == 1;   // LessEqual
    pass = pass && compare_conditions(5, 2.1f, 2.0f) == 0;
    pass = pass && compare_conditions(6, 0.0f, 999.0f) == 1; // always pass
    pass = pass && compare_conditions(7, 999.0f, 0.0f) == 1; // always pass

    record(name, pass, pass ? "" : "comparison operator semantics mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_pass_and_fail() {
    const double t0 = nowMs();
    const std::string name = "evaluate_single pass / hard fail";

    const uint32_t run_as = 0x1111;
    const uint32_t target = 0x2222;

    FakeRuntime rt;
    rt.query_results[kFnSecondsPassed] = 1.0f;
    const ConditionItem item =
        make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed);
    const EvalResult hit = evaluate_single(item, run_as, target, rt);
    const bool subject_is_run_as = (rt.last_subject == run_as);

    rt.query_results[kFnSecondsPassed] = 2.0f;
    const EvalResult miss = evaluate_single(item, run_as, target, rt);

    const bool pass = hit.passed && !hit.skip && subject_is_run_as &&
                      !miss.passed && !miss.skip;
    record(name, pass, pass ? "" : "pass/fail or subject selection mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_query_failure_skip() {
    const double t0 = nowMs();
    const std::string name = "runtime query failure maps to skip";

    FakeRuntime rt;
    rt.query_failures.insert(kFnSecondsPassed);
    const ConditionItem item =
        make_item(make_flag(false, false, false, 0), float_bits(0.0f), kFnSecondsPassed);
    const EvalResult r = evaluate_single(item, 0x1111, 0x2222, rt);

    const bool pass = !r.passed && r.skip;
    record(name, pass, pass ? "" : "expected {passed=false, skip=true}",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_unknown_function_skip() {
    const double t0 = nowMs();
    const std::string name = "unknown function index skips without query";

    FakeRuntime rt;
    const ConditionItem item =
        make_item(make_flag(false, false, false, 0), float_bits(0.0f), kFnUnknown);
    const EvalResult r = evaluate_single(item, 0x1111, 0x2222, rt);

    const bool pass = !r.passed && r.skip && rt.query_count == 0;
    record(name, pass, pass ? "" : "expected skip with no runtime query",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_swap_operand() {
    const double t0 = nowMs();
    const std::string name = "swapped condition feeds compare_target as operand";

    const uint32_t run_as = 0x1111;
    const uint32_t target = 0x2222;

    FakeRuntime rt;
    const ConditionItem swapped =
        make_item(make_flag(false, true, false, 0), float_bits(0.0f), kFnSecondsPassed);
    evaluate_single(swapped, run_as, target, rt);
    const bool swap_ok = (rt.last_subject == target);

    rt.query_count = 0;
    const ConditionItem normal =
        make_item(make_flag(false, false, false, 0), float_bits(0.0f), kFnSecondsPassed);
    evaluate_single(normal, run_as, target, rt);
    const bool normal_ok = (rt.last_subject == run_as);

    const bool pass = swap_ok && normal_ok;
    record(name, pass, pass ? "" : "operand swap mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_null_param_fallback() {
    const double t0 = nowMs();
    const std::string name = "null param1 falls back to operand form id";

    const uint32_t run_as = 0x1111;
    const uint32_t target = 0x2222;

    FakeRuntime rt;
    // GetStage (58) takes a parameter; param1 == 0 substitutes the operand side.
    const ConditionItem unswapped =
        make_item(make_flag(false, false, false, 0), float_bits(0.0f), kFnStage);
    evaluate_single(unswapped, run_as, target, rt);
    const bool fallback_target = (rt.last_param1 == target);

    const ConditionItem swapped =
        make_item(make_flag(false, true, false, 0), float_bits(0.0f), kFnStage);
    evaluate_single(swapped, run_as, target, rt);
    const bool fallback_run_as = (rt.last_param1 == run_as);

    const ConditionItem explicit_param =
        make_item(make_flag(false, false, false, 0), float_bits(0.0f), kFnStage, 0x77);
    evaluate_single(explicit_param, run_as, target, rt);
    const bool explicit_kept = (rt.last_param1 == 0x77);

    const bool pass = fallback_target && fallback_run_as && explicit_kept;
    record(name, pass, pass ? "" : "param1 fallback mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_needs_ref_guard() {
    const double t0 = nowMs();
    const std::string name = "needs_ref function with null operand hard-fails";

    FakeRuntime rt;
    rt.query_results[kFnDistance] = 100.0f;
    const ConditionItem item =
        make_item(make_flag(false, false, false, 4), float_bits(50.0f), kFnDistance);
    // GetDistance needs a reference; unswapped operand is run_as == 0.
    const EvalResult r = evaluate_single(item, 0, 0x2222, rt);

    const bool pass = !r.passed && !r.skip && rt.query_count == 0;
    record(name, pass, pass ? "" : "expected hard fail before query",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_global_compare() {
    const double t0 = nowMs();
    const std::string name = "uses_global sources right side from global_value";

    const uint32_t global_id = 0x123456;
    FakeRuntime rt;
    rt.globals[global_id] = 5.0f;
    rt.query_results[kFnSecondsPassed] = 5.0f;

    const ConditionItem item =
        make_item(make_flag(false, false, true, 0), global_id, kFnSecondsPassed);
    const EvalResult hit = evaluate_single(item, 0x1111, 0x2222, rt);

    // comparison_value == 0 disables the global lookup; right side is the raw float bits.
    FakeRuntime rt2;
    rt2.query_results[kFnSecondsPassed] = 0.0f;
    const ConditionItem zero =
        make_item(make_flag(false, false, true, 0), 0, kFnSecondsPassed);
    const EvalResult zero_hit = evaluate_single(zero, 0x1111, 0x2222, rt2);

    const bool pass = hit.passed && !hit.skip && rt.global_calls == 1 &&
                      zero_hit.passed && rt2.global_calls == 0;
    record(name, pass, pass ? "" : "global compare mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_eval_disposition_skip_marker() {
    const double t0 = nowMs();
    const std::string name = "GetDisposition greater ops fail as skip";

    // RE FUN_0056ac50: GetDisposition (0x4C) failures with op Greater/GreaterEqual
    // are reported as skip; other ops hard-fail.
    FakeRuntime rt;  // query result 0.0f, compare 1.0f -> all ops fail

    const ConditionItem greater =
        make_item(make_flag(false, false, false, 2), float_bits(1.0f), kFnDisposition);
    const EvalResult g = evaluate_single(greater, 0x1111, 0x2222, rt);

    const ConditionItem ge =
        make_item(make_flag(false, false, false, 3), float_bits(1.0f), kFnDisposition);
    const EvalResult e = evaluate_single(ge, 0x1111, 0x2222, rt);

    const ConditionItem equal =
        make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnDisposition);
    const EvalResult q = evaluate_single(equal, 0x1111, 0x2222, rt);

    const bool pass = !g.passed && g.skip && !e.passed && e.skip &&
                      !q.passed && !q.skip;
    record(name, pass, pass ? "" : "disposition op-2/3 skip marker mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_empty_and_null() {
    const double t0 = nowMs();
    const std::string name = "empty / null condition list passes";

    FakeRuntime rt;
    std::vector<ConditionItem> empty;
    const EvalResult a = evaluate_list(&empty, 0x1111, 0x2222, rt);
    const EvalResult b = evaluate_list(nullptr, 0x1111, 0x2222, rt);

    const bool pass = a.passed && !a.skip && b.passed && !b.skip && rt.query_count == 0;
    record(name, pass, pass ? "" : "empty list should pass untouched",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_and_short_circuit() {
    const double t0 = nowMs();
    const std::string name = "AND list short-circuits on hard fail";

    FakeRuntime rt;
    rt.query_results[kFnSecondsPassed] = 1.0f;
    std::vector<ConditionItem> list;
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(9.0f), kFnSecondsPassed));
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult r = evaluate_list(&list, 0x1111, 0x2222, rt);

    // Item 1 fails hard (Equal 1.0 vs 9.0) -> early return, third item never queried.
    const bool fail_path = !r.passed && !r.skip && rt.query_count == 2;

    FakeRuntime rt2;
    rt2.query_results[kFnSecondsPassed] = 1.0f;
    std::vector<ConditionItem> ok_list;
    ok_list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    ok_list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult p = evaluate_list(&ok_list, 0x1111, 0x2222, rt2);

    const bool pass = fail_path && p.passed && !p.skip;
    record(name, pass, pass ? "" : "AND accumulation mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_or_group_close_pass() {
    const double t0 = nowMs();
    const std::string name = "OR group passes via closing member";

    FakeRuntime rt;
    rt.query_results[kFnSecondsPassed] = 1.0f;
    std::vector<ConditionItem> list;
    // OR-start passes, non-OR closer passes -> group closes with and_acc=1.
    list.push_back(make_item(make_flag(true, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult r = evaluate_list(&list, 0x1111, 0x2222, rt);

    const bool pass = r.passed && !r.skip;
    record(name, pass, pass ? "" : "expected OR group pass",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_or_group_close_fail() {
    const double t0 = nowMs();
    const std::string name = "OR group hard-fails when closer fails";

    FakeRuntime rt;  // query result 0.0f vs compare 1.0f -> items fail hard
    std::vector<ConditionItem> list;
    list.push_back(make_item(make_flag(true, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult r = evaluate_list(&list, 0x1111, 0x2222, rt);

    const bool pass = !r.passed && !r.skip;
    record(name, pass, pass ? "" : "expected hard fail, no skip",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_or_group_all_skip() {
    const double t0 = nowMs();
    const std::string name = "OR group of skipped items results in skip";

    FakeRuntime rt;
    rt.query_failures.insert(kFnSecondsPassed);
    std::vector<ConditionItem> list;
    list.push_back(make_item(make_flag(true, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult r = evaluate_list(&list, 0x1111, 0x2222, rt);

    const bool pass = !r.passed && r.skip;
    record(name, pass, pass ? "" : "expected skip result",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_or_group_skip_then_pass() {
    const double t0 = nowMs();
    const std::string name = "OR group skipped start with passing closer passes";

    // RE FUN_0056a510: a skip on the OR-start does not poison the group when a
    // later non-OR member passes.
    FakeRuntime rt;
    rt.query_failures.insert(kFnUnknown);  // OR-start item cannot be evaluated
    rt.query_results[kFnSecondsPassed] = 1.0f;
    std::vector<ConditionItem> list;
    list.push_back(make_item(make_flag(true, false, false, 0), float_bits(1.0f), kFnUnknown));
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult r = evaluate_list(&list, 0x1111, 0x2222, rt);

    const bool pass = r.passed && !r.skip;
    record(name, pass, pass ? "" : "expected pass despite skipped group start",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_secondary_adoption() {
    const double t0 = nowMs();
    const std::string name = "secondary list adopted once after primary";

    FakeRuntime rt;
    rt.query_results[kFnSecondsPassed] = 1.0f;
    std::vector<ConditionItem> primary;
    primary.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    std::vector<ConditionItem> secondary;
    secondary.push_back(make_item(make_flag(false, false, false, 0), float_bits(9.0f), kFnSecondsPassed));
    const EvalResult r = evaluate_list(&primary, 0x1111, 0x2222, rt, &secondary);

    // Primary passes, secondary item fails hard -> overall fail after adoption.
    const bool adopt_fail = !r.passed && !r.skip && rt.query_count == 2;

    // Empty primary never triggers adoption (processed_any guard) -> pass untouched.
    FakeRuntime rt2;
    std::vector<ConditionItem> empty;
    std::vector<ConditionItem> sec;
    sec.push_back(make_item(make_flag(false, false, false, 0), float_bits(1.0f), kFnSecondsPassed));
    const EvalResult p = evaluate_list(&empty, 0x1111, 0x2222, rt2, &sec);
    const bool no_adopt = p.passed && rt2.query_count == 0;

    const bool pass = adopt_fail && no_adopt;
    record(name, pass, pass ? "" : "secondary adoption mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_list_context_override() {
    const double t0 = nowMs();
    const std::string name = "context_override replaces compare target";

    const uint32_t target = 0x2222;
    const uint32_t override_id = 0x3333;

    std::vector<ConditionItem> list;
    list.push_back(make_item(make_flag(false, false, false, 0), float_bits(0.0f), kFnStage));

    FakeRuntime rt;
    rt.override_value = override_id;
    evaluate_list(&list, 0x1111, target, rt);
    const bool overridden = (rt.last_param1 == override_id);  // param1=0 falls back to target

    FakeRuntime rt2;
    rt2.override_value = 0;
    evaluate_list(&list, 0x1111, target, rt2);
    const bool untouched = (rt2.last_param1 == target);

    const bool pass = overridden && untouched;
    record(name, pass, pass ? "" : "context override plumbing mismatch",
           static_cast<float>(nowMs() - t0));
}

void ConditionTests::test_table_metadata() {
    const double t0 = nowMs();
    const std::string name = "kCondFunctions table metadata";

    const bool count_ok = (kCondFunctionCount == 169);

    const game::CondFuncEntry* seconds = find_cond_function(kFnSecondsPassed);
    const bool seconds_ok = seconds != nullptr && seconds->index == kFnSecondsPassed &&
                            seconds->param_count == 0 && seconds->needs_ref == 0 &&
                            trim_name(seconds->name) == "GetSecondsPassed";

    const game::CondFuncEntry* distance = find_cond_function(kFnDistance);
    const bool distance_ok = distance != nullptr && distance->param_count == 1 &&
                             distance->needs_ref == 1;

    const game::CondFuncEntry* disposition = find_cond_function(kFnDisposition);
    const bool disposition_ok = disposition != nullptr && disposition->needs_ref == 1;

    const bool absent_ok = (find_cond_function(2) == nullptr);

    const game::CondFuncEntry* last = find_cond_function(365);
    const bool last_ok = last != nullptr && trim_name(last->name) == "GetPlayerInSEWorld";

    const bool pass = count_ok && seconds_ok && distance_ok && disposition_ok &&
                      absent_ok && last_ok;
    record(name, pass, pass ? "" : "table value mismatch",
           static_cast<float>(nowMs() - t0));
}
