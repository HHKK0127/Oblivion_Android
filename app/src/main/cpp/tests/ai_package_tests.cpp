#include "ai_package_tests.h"

#include "../game/ai_package.h"
#include "../game/condition_system.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

using ai::AIPackage;
using ai::PackageEvalContext;
using ai::PackageSchedule;
using ai::PackageStack;
using ai::PackageType;
using game::ConditionItem;
using game::ConditionRuntime;

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
    return static_cast<uint8_t>((is_or ? 1u : 0u) |
                                (swapped ? 2u : 0u) |
                                (uses_global ? 4u : 0u) |
                                ((op & 7u) << 5));
}

ConditionItem make_item(uint8_t flag, uint32_t cmp, uint16_t fn,
                        uint32_t p1 = 0, uint32_t p2 = 0) {
    ConditionItem item{};
    item.flag = flag;
    item.comparison_value = cmp;
    item.function_index = fn;
    item.param1 = p1;
    item.param2 = p2;
    return item;
}

// Table rows verified against tmp/re/s58_condfunctions_compact.txt:
//   12 GetSecondsPassed  param_count=0 needs_ref=0
//   58 GetStage          param_count=1 needs_ref=0
//   999 is absent from the 169-entry table (unknown function -> skip).
constexpr uint16_t kFnSecondsPassed = 12;
constexpr uint16_t kFnStage = 58;
constexpr uint16_t kFnUnknown = 999;

class PackageTestRuntime : public ConditionRuntime {
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

    float global_value(uint32_t id) override {
        ++global_calls;
        const auto it = globals.find(id);
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

AIPackage make_plain_package(uint32_t id, uint8_t priority) {
    AIPackage pkg;
    pkg.packageId = id;
    pkg.type = PackageType::WANDER;
    pkg.priority = priority;
    return pkg;
}

}  // namespace

bool AiPackageTests::runAllTests() {
    results.clear();
    std::cout << "[AiPackageTests] Running AI package selection tests..." << std::endl;
    test_first_match_scan_order();
    test_no_eligible_package_returns_nullptr();
    test_schedule_window_midnight_wrap();
    test_schedule_day_mask();
    test_schedule_zero_duration_never_matches();
    test_disabled_schedule_always_matches();
    test_ctda_and_pass_and_fail();
    test_ctda_skip_is_accepted();
    test_ctda_empty_list_passes();
    test_ctda_without_runtime_passes();
    test_chain_blocks_downstream_stage();
    const int pass = getPassCount();
    const int fail = getFailCount();
    std::cout << "[AiPackageTests] Completed: " << pass << " passed, " << fail
              << " failed" << std::endl;
    return fail == 0;
}

int AiPackageTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (r.passed) ++count;
    }
    return count;
}

int AiPackageTests::getFailCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (!r.passed) ++count;
    }
    return count;
}

std::string AiPackageTests::getSummary() const {
    std::ostringstream out;
    out << "AI Package Test Results\n";
    out << "Total: " << results.size() << " | Pass: " << getPassCount()
        << " | Fail: " << getFailCount();
    return out.str();
}

void AiPackageTests::record(const std::string& name, bool passed,
                            const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
    std::cout << "[AiPackageTests] " << (passed ? "PASS" : "FAIL") << " " << name;
    if (!msg.empty()) {
        std::cout << " - " << msg;
    }
    std::cout << " (" << ms << " ms)" << std::endl;
}

// ---------------------------------------------------------------------------
// Canon 0x569020: first eligible package in list order wins. pushPackage keeps
// the list priority-descending (documented port deviation), so the highest
// priority package is the first match when all gates pass.
// ---------------------------------------------------------------------------
void AiPackageTests::test_first_match_scan_order() {
    const double t0 = nowMs();
    PackageStack stack;
    stack.pushPackage(ai::PackageFactory::createWander(1, 5.0f, 30));
    stack.pushPackage(ai::PackageFactory::createCombat(2, 0xABCD, 90));

    bool ok = (stack.size() == 2);
    ok = ok && stack.getPackages()[0].packageId == 2 &&
         stack.getPackages()[1].packageId == 1;

    PackageEvalContext ctx;  // defaults: hour 12, no schedule, no CTDA
    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    ok = ok && active != nullptr && active->packageId == 2;

    record("first_match_scan_order", ok,
           ok ? "" : "first-match must return highest-priority eligible package",
           static_cast<float>(nowMs() - t0));
}

// A package whose time condition fails is never eligible; with no other
// candidates the stack returns nullptr (no packages[0] fallback).
void AiPackageTests::test_no_eligible_package_returns_nullptr() {
    const double t0 = nowMs();
    PackageStack stack;
    // Sleep window [9,10); evaluate at hour 20 -> outside window.
    stack.pushPackage(
        ai::PackageFactory::createSleep(10, glm::vec3(0.0f, 0.0f, 0.0f), 9, 10, 60));

    PackageEvalContext ctx;
    ctx.hourOfDay = 20.0f;
    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    const bool ok = (active == nullptr);
    record("no_eligible_package_returns_nullptr", ok,
           ok ? "" : "ineligible package must not be returned",
           static_cast<float>(nowMs() - t0));
}

// Canon 0x568EB0: window is [start, start+duration) mod 24, half-open.
void AiPackageTests::test_schedule_window_midnight_wrap() {
    const double t0 = nowMs();
    PackageSchedule sched;
    sched.enabled = true;
    sched.startHour = 22.0f;
    sched.durationHours = 4.0f;  // wraps midnight -> [22,2)
    sched.dayMask = 0x7F;

    bool ok = sched.matches(23.0f, 0);   // inside, before wrap
    ok = ok && sched.matches(1.0f, 0);   // inside, after wrap
    ok = ok && !sched.matches(20.0f, 0); // before start
    ok = ok && !sched.matches(2.0f, 0);  // end is exclusive
    ok = ok && sched.matches(22.0f, 0);  // start is inclusive
    record("schedule_window_midnight_wrap", ok,
           ok ? "" : "wrapped window must be [22,2) half-open",
           static_cast<float>(nowMs() - t0));
}

// dayMask bit (1 << dayOfWeek); dayOfWeek >= 7 never matches.
void AiPackageTests::test_schedule_day_mask() {
    const double t0 = nowMs();
    PackageSchedule sched;
    sched.enabled = true;
    sched.startHour = 0.0f;
    sched.durationHours = 24.0f;

    sched.dayMask = 0x02;  // day 1 only
    bool ok = sched.matches(12.0f, 1);
    ok = ok && !sched.matches(12.0f, 0);
    ok = ok && !sched.matches(12.0f, 2);

    sched.dayMask = 0x7F;  // all days
    for (uint8_t day = 0; day <= 6 && ok; ++day) {
        ok = ok && sched.matches(12.0f, day);
    }
    ok = ok && !sched.matches(12.0f, 7);  // out of range
    record("schedule_day_mask", ok, ok ? "" : "day mask bits must gate dayOfWeek",
           static_cast<float>(nowMs() - t0));
}

// durationHours <= 0 never matches; >= 24 always matches (after day check).
void AiPackageTests::test_schedule_zero_duration_never_matches() {
    const double t0 = nowMs();
    PackageSchedule sched;
    sched.enabled = true;
    sched.startHour = 8.0f;
    sched.durationHours = 0.0f;
    sched.dayMask = 0x7F;

    bool ok = !sched.matches(8.0f, 0) && !sched.matches(0.0f, 0);

    sched.durationHours = 24.0f;
    ok = ok && sched.matches(8.0f, 0) && sched.matches(23.9f, 6);
    record("schedule_zero_duration_never_matches", ok,
           ok ? "" : "zero duration never matches, 24h always matches",
           static_cast<float>(nowMs() - t0));
}

// Disabled schedule is transparent (always matches).
void AiPackageTests::test_disabled_schedule_always_matches() {
    const double t0 = nowMs();
    PackageSchedule sched;  // default: enabled=false
    const bool ok = sched.matches(0.0f, 6) && sched.matches(23.5f, 3) &&
                    sched.matches(12.0f, 7);
    record("disabled_schedule_always_matches", ok,
           ok ? "" : "disabled schedule must always match",
           static_cast<float>(nowMs() - t0));
}

// CTDA list results feed conditionsMet: all-pass -> eligible, hard fail -> not.
void AiPackageTests::test_ctda_and_pass_and_fail() {
    const double t0 = nowMs();
    PackageTestRuntime rt;
    rt.query_results[kFnSecondsPassed] = 5.0f;
    rt.query_results[kFnStage] = 7.0f;

    AIPackage pkg = make_plain_package(100, 50);
    pkg.ctdaConditions.push_back(
        make_item(make_flag(false, false, false, 0 /*Equal*/),
                  float_bits(5.0f), kFnSecondsPassed));
    pkg.ctdaConditions.push_back(
        make_item(make_flag(false, false, false, 0 /*Equal*/),
                  float_bits(7.0f), kFnStage));

    PackageStack stack;
    stack.pushPackage(pkg);
    PackageEvalContext ctx;
    ctx.runtime = &rt;

    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    bool ok = (active != nullptr && active->packageId == 100);

    // Second condition now fails hard (AND group) -> no eligible package.
    rt.query_results[kFnStage] = 1.0f;
    stack.evaluate(ctx);
    ok = ok && (stack.getActivePackage() == nullptr);

    // First condition fails hard as well.
    rt.query_results[kFnSecondsPassed] = 3.0f;
    stack.evaluate(ctx);
    ok = ok && (stack.getActivePackage() == nullptr);

    record("ctda_and_pass_and_fail", ok,
           ok ? "" : "AND group: any hard fail must block eligibility",
           static_cast<float>(nowMs() - t0));
}

// Unknown function index -> evaluate_single returns skip; the port treats skip
// as acceptance (documented deviation from canon).
void AiPackageTests::test_ctda_skip_is_accepted() {
    const double t0 = nowMs();
    PackageTestRuntime rt;

    AIPackage pkg = make_plain_package(101, 50);
    pkg.ctdaConditions.push_back(
        make_item(make_flag(false, false, false, 0), float_bits(5.0f), kFnUnknown));

    PackageStack stack;
    stack.pushPackage(pkg);
    PackageEvalContext ctx;
    ctx.runtime = &rt;

    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    const bool ok = (active != nullptr && active->packageId == 101);
    record("ctda_skip_is_accepted", ok,
           ok ? "" : "skip result must be treated as pass",
           static_cast<float>(nowMs() - t0));
}

// Empty CTDA list skips the condition stage entirely.
void AiPackageTests::test_ctda_empty_list_passes() {
    const double t0 = nowMs();
    PackageTestRuntime rt;

    AIPackage pkg = make_plain_package(102, 50);  // ctdaConditions empty
    PackageStack stack;
    stack.pushPackage(pkg);
    PackageEvalContext ctx;
    ctx.runtime = &rt;

    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    const bool ok = (active != nullptr && active->packageId == 102 &&
                     rt.query_count == 0);
    record("ctda_empty_list_passes", ok,
           ok ? "" : "empty CTDA list must pass without querying runtime",
           static_cast<float>(nowMs() - t0));
}

// With no runtime bridge yet, a non-empty CTDA list is ignored (documented
// deviation) so packages remain eligible rather than being wrongly blocked.
void AiPackageTests::test_ctda_without_runtime_passes() {
    const double t0 = nowMs();
    AIPackage pkg = make_plain_package(103, 50);
    pkg.ctdaConditions.push_back(
        make_item(make_flag(false, false, false, 0), float_bits(5.0f),
                  kFnSecondsPassed));

    PackageStack stack;
    stack.pushPackage(pkg);
    PackageEvalContext ctx;
    ctx.runtime = nullptr;

    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    const bool ok = (active != nullptr && active->packageId == 103);
    record("ctda_without_runtime_passes", ok,
           ok ? "" : "non-empty CTDA list without runtime must pass",
           static_cast<float>(nowMs() - t0));
}

// Canon chain order: flag gates -> schedule -> CTDA. A schedule mismatch must
// block a package whose CTDA list would otherwise pass, and a CTDA hard fail
// must block a package whose schedule matches.
void AiPackageTests::test_chain_blocks_downstream_stage() {
    const double t0 = nowMs();
    PackageTestRuntime rt;
    rt.query_results[kFnSecondsPassed] = 5.0f;

    // (a) Schedule mismatch blocks downstream CTDA pass.
    AIPackage pkg = make_plain_package(104, 50);
    pkg.schedule.enabled = true;
    pkg.schedule.startHour = 9.0f;
    pkg.schedule.durationHours = 1.0f;  // [9,10), ctx hour 12 -> mismatch
    pkg.ctdaConditions.push_back(
        make_item(make_flag(false, false, false, 0), float_bits(5.0f),
                  kFnSecondsPassed));

    PackageStack stack;
    stack.pushPackage(pkg);
    PackageEvalContext ctx;
    ctx.runtime = &rt;  // hourOfDay defaults to 12.0

    stack.evaluate(ctx);
    bool ok = (stack.getActivePackage() == nullptr);

    // (b) Schedule matches but CTDA fails hard.
    stack.clear();
    pkg.schedule.startHour = 11.0f;
    pkg.schedule.durationHours = 2.0f;  // [11,13) contains 12
    rt.query_results[kFnSecondsPassed] = 3.0f;  // Equal 5.0 fails
    stack.pushPackage(pkg);
    stack.evaluate(ctx);
    ok = ok && (stack.getActivePackage() == nullptr);

    // (c) Positive control: both gates pass.
    stack.clear();
    rt.query_results[kFnSecondsPassed] = 5.0f;
    stack.pushPackage(pkg);
    stack.evaluate(ctx);
    const AIPackage* active = stack.getActivePackage();
    ok = ok && active != nullptr && active->packageId == 104;

    record("chain_blocks_downstream_stage", ok,
           ok ? "" : "schedule and CTDA stages must both gate eligibility",
           static_cast<float>(nowMs() - t0));
}
