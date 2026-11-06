// Host tests for TopicResolver: parity of native Topic_Resolve (FUN_0052F010).
// Covers disabled early exit, ordered scan (immediate / deferred / priority-stop),
// seeded lottery reproducibility, last-eligible fallback, and the recursion branch.

#include "topic_resolve_tests.h"

#include <string>
#include <vector>

#include "../dialogue/topic_resolver.h"

namespace oblivion::dialogue {

namespace {

TopicCandidate makeCandidate(uint32_t formID, bool canUse, bool alternate = false, bool priorityStop = false) {
    TopicCandidate candidate;
    candidate.formID = formID;
    candidate.canUse = canUse;
    candidate.alternate = alternate;
    candidate.priorityStop = priorityStop;
    return candidate;
}

TopicResolveInput makeInput(std::vector<TopicCandidate> candidates) {
    TopicResolveInput input;
    input.candidates = std::move(candidates);
    return input;
}

}  // namespace

bool TopicResolveTests::runAllTests() {
    results.clear();
    testDisabledReturnsNothing();
    testFirstEligibleWins();
    testSkipsIneligible();
    testNoEligibleReturnsNothing();
    testEmptyCandidates();
    testAlternateDefersToPlain();
    testSeededLotteryReproducible();
    testLotteryStaysWithinDeferred();
    testPriorityStopAbortsScan();
    testPriorityStopBeforeLaterPlain();
    testPriorityStopAloneFallback();
    testRecursionGuardFires();
    testRecursionTypeGate();
    testDisabledBlocksRecursion();
    return getFailCount() == 0;
}

int TopicResolveTests::getPassCount() const {
    int count = 0;
    for (const auto& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int TopicResolveTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string TopicResolveTests::getSummary() const {
    return std::to_string(getPassCount()) + "/" + std::to_string(results.size()) + " passed";
}

void TopicResolveTests::record(const std::string& name, bool passed, const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

// bit5 of [this+8] set -> early exit, no selection at all.
void TopicResolveTests::testDisabledReturnsNothing() {
    TopicResolveInput input = makeInput({makeCandidate(1, true)});
    input.disabled = true;
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == -1 && !result.usedRecursion;
    record("DisabledReturnsNothing", ok, "selectedIndex=" + std::to_string(result.selectedIndex));
}

// First eligible non-alternate candidate wins immediately.
void TopicResolveTests::testFirstEligibleWins() {
    TopicResolveInput input = makeInput({makeCandidate(1, true), makeCandidate(2, true)});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == 0 && result.deferredCount == 0;
    record("FirstEligibleWins", ok, "selectedIndex=" + std::to_string(result.selectedIndex));
}

// Ineligible candidates are skipped by the scan.
void TopicResolveTests::testSkipsIneligible() {
    TopicResolveInput input = makeInput({makeCandidate(1, false), makeCandidate(2, true)});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == 1;
    record("SkipsIneligible", ok, "selectedIndex=" + std::to_string(result.selectedIndex));
}

// No eligible candidate -> nothing selected.
void TopicResolveTests::testNoEligibleReturnsNothing() {
    TopicResolveInput input = makeInput({makeCandidate(1, false), makeCandidate(2, false, true)});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == -1 && result.deferredCount == 0;
    record("NoEligibleReturnsNothing", ok, "selectedIndex=" + std::to_string(result.selectedIndex));
}

// Empty candidate list -> nothing selected.
void TopicResolveTests::testEmptyCandidates() {
    TopicResolveInput input = makeInput({});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == -1;
    record("EmptyCandidates", ok, "selectedIndex=" + std::to_string(result.selectedIndex));
}

// bit1 candidate is deferred; a later plain eligible candidate is selected immediately.
void TopicResolveTests::testAlternateDefersToPlain() {
    TopicResolveInput input = makeInput({makeCandidate(1, true, true), makeCandidate(2, true)});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == 1 && result.deferredCount == 1;
    record("AlternateDefersToPlain", ok,
           "selectedIndex=" + std::to_string(result.selectedIndex) +
           " deferredCount=" + std::to_string(result.deferredCount));
}

// Deferred-only pool: same seed on two resolvers yields the same lottery draw.
void TopicResolveTests::testSeededLotteryReproducible() {
    TopicResolveInput input = makeInput({makeCandidate(1, true, true), makeCandidate(2, true, true), makeCandidate(3, true, true)});
    TopicResolver first;
    TopicResolver second;
    first.setSeed(42);
    second.setSeed(42);
    const TopicResolveResult a = first.resolve(input);
    const TopicResolveResult b = second.resolve(input);
    const bool ok = a.selectedIndex == b.selectedIndex && a.selectedIndex >= 0 && a.selectedIndex <= 2 &&
                    a.deferredCount == 3;
    record("SeededLotteryReproducible", ok,
           "first=" + std::to_string(a.selectedIndex) + " second=" + std::to_string(b.selectedIndex));
}

// All draws land inside the deferred index set; ineligible candidates never appear.
void TopicResolveTests::testLotteryStaysWithinDeferred() {
    TopicResolveInput input = makeInput({
        makeCandidate(1, true, true),
        makeCandidate(2, false, true),
        makeCandidate(3, true, true),
        makeCandidate(4, true, true),
    });
    TopicResolver resolver;
    resolver.setSeed(7);
    bool ok = true;
    bool sawDifferent = false;
    int previous = -1;
    for (int i = 0; i < 20; ++i) {
        const TopicResolveResult result = resolver.resolve(input);
        if (result.deferredCount != 3) {
            ok = false;
        }
        if (result.selectedIndex != 0 && result.selectedIndex != 2 && result.selectedIndex != 3) {
            ok = false;
        }
        if (previous >= 0 && result.selectedIndex != previous) {
            sawDifferent = true;
        }
        previous = result.selectedIndex;
    }
    // A lottery over three candidates should not be constant across 20 draws.
    ok = ok && sawDifferent;
    record("LotteryStaysWithinDeferred", ok, ok ? "" : "draw outside deferred set or never varied");
}

// bit5 aborts the scan: earlier deferred pool is resolved by lottery, later candidates ignored.
void TopicResolveTests::testPriorityStopAbortsScan() {
    TopicResolveInput input = makeInput({
        makeCandidate(1, true, true),
        makeCandidate(2, true, false, true),
        makeCandidate(3, true),
    });
    TopicResolver resolver;
    resolver.setSeed(123);
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.priorityStopTriggered && result.deferredCount == 1 && result.selectedIndex == 0;
    record("PriorityStopAbortsScan", ok,
           "selectedIndex=" + std::to_string(result.selectedIndex) +
           " deferredCount=" + std::to_string(result.deferredCount) +
           " stop=" + std::to_string(result.priorityStopTriggered));
}

// bit5 candidate with an empty deferred pool is selected as the last eligible.
void TopicResolveTests::testPriorityStopBeforeLaterPlain() {
    TopicResolveInput input = makeInput({makeCandidate(1, true, false, true), makeCandidate(2, true)});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.priorityStopTriggered && result.selectedIndex == 0 && result.deferredCount == 0;
    record("PriorityStopBeforeLaterPlain", ok,
           "selectedIndex=" + std::to_string(result.selectedIndex));
}

// Priority stop after an ineligible candidate: fallback still points at it.
void TopicResolveTests::testPriorityStopAloneFallback() {
    TopicResolveInput input = makeInput({makeCandidate(1, false), makeCandidate(2, true, false, true)});
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.priorityStopTriggered && result.selectedIndex == 1;
    record("PriorityStopAloneFallback", ok,
           "selectedIndex=" + std::to_string(result.selectedIndex));
}

// Guard set + allowRecursivePass + conversation type in {2,3,4,6} -> recursion branch runs.
void TopicResolveTests::testRecursionGuardFires() {
    TopicResolveInput input = makeInput({makeCandidate(1, true)});
    input.recursionGuard = true;
    input.allowRecursivePass = true;
    input.conversationType = 3;
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.usedRecursion && result.selectedIndex == 0;
    record("RecursionGuardFires", ok,
           "usedRecursion=" + std::to_string(result.usedRecursion) +
           " selectedIndex=" + std::to_string(result.selectedIndex));
}

// Type outside {2,3,4,6} and allowRecursivePass=false must not recurse.
void TopicResolveTests::testRecursionTypeGate() {
    TopicResolveInput gated = makeInput({makeCandidate(1, true)});
    gated.recursionGuard = true;
    gated.allowRecursivePass = true;
    gated.conversationType = 5;
    TopicResolver resolver;
    const TopicResolveResult gateResult = resolver.resolve(gated);

    TopicResolveInput notAllowed = makeInput({makeCandidate(1, true)});
    notAllowed.recursionGuard = true;
    notAllowed.allowRecursivePass = false;
    notAllowed.conversationType = 3;
    const TopicResolveResult allowResult = resolver.resolve(notAllowed);

    const bool ok = !gateResult.usedRecursion && gateResult.selectedIndex == 0 &&
                    !allowResult.usedRecursion && allowResult.selectedIndex == 0;
    record("RecursionTypeGate", ok,
           "gate=" + std::to_string(gateResult.usedRecursion) +
           " allow=" + std::to_string(allowResult.usedRecursion));
}

// Disabled early exit takes precedence over the recursion branch.
void TopicResolveTests::testDisabledBlocksRecursion() {
    TopicResolveInput input = makeInput({makeCandidate(1, true)});
    input.disabled = true;
    input.recursionGuard = true;
    input.allowRecursivePass = true;
    input.conversationType = 2;
    TopicResolver resolver;
    const TopicResolveResult result = resolver.resolve(input);
    const bool ok = result.selectedIndex == -1 && !result.usedRecursion;
    record("DisabledBlocksRecursion", ok,
           "selectedIndex=" + std::to_string(result.selectedIndex) +
           " usedRecursion=" + std::to_string(result.usedRecursion));
}

}  // namespace oblivion::dialogue
