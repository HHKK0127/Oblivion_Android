#ifndef TESTS_PLAYER_CHARACTER_STATE_TESTS_H
#define TESTS_PLAYER_CHARACTER_STATE_TESTS_H

#include <string>
#include <vector>

namespace game {

struct PlayerCharacterStateTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

// Host tests for the DAT_00B333C4 player-character global port
// (v72_global_b333c4_map.md canon).
class PlayerCharacterStateTests {
public:
    PlayerCharacterStateTests() = default;

    bool runAllTests();
    const std::vector<PlayerCharacterStateTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void testSizeIs804();
    void testInitSetsCtorBytes();
    void testDoubleInitIdempotent();
    void testDestroyClearsGlobal();
    void testNullSafetyBeforeCreate();
    void testLifecycleIsValid();
    void testRecursionGuardRoundTrip();
    void testPositionFloatsRoundTrip();
    void testInCombatFlagByte();
    void testDifficultyDefault();
    void testTimeCounterIncrement();
    void testDynamicBandRoundTrip();
    void testDynamicBandOutOfRangeRejected();
    void testViewStateFlagDefaultThirdPerson();
    void testViewStateFlagRoundTrip();

    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);

    std::vector<PlayerCharacterStateTestResult> results;
};

}  // namespace game

#endif  // TESTS_PLAYER_CHARACTER_STATE_TESTS_H
