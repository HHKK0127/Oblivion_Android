#ifndef TESTS_TOPIC_RESOLVE_TESTS_H
#define TESTS_TOPIC_RESOLVE_TESTS_H

#include <string>
#include <vector>

namespace oblivion::dialogue {

struct TopicResolveTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

// Host tests for TopicResolver: parity of native Topic_Resolve (FUN_0052F010).
class TopicResolveTests {
public:
    TopicResolveTests() = default;

    bool runAllTests();
    const std::vector<TopicResolveTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void testDisabledReturnsNothing();
    void testFirstEligibleWins();
    void testSkipsIneligible();
    void testNoEligibleReturnsNothing();
    void testEmptyCandidates();
    void testAlternateDefersToPlain();
    void testSeededLotteryReproducible();
    void testLotteryStaysWithinDeferred();
    void testPriorityStopAbortsScan();
    void testPriorityStopBeforeLaterPlain();
    void testPriorityStopAloneFallback();
    void testRecursionGuardFires();
    void testRecursionTypeGate();
    void testDisabledBlocksRecursion();

    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);

    std::vector<TopicResolveTestResult> results;
};

}  // namespace oblivion::dialogue

#endif  // TESTS_TOPIC_RESOLVE_TESTS_H
