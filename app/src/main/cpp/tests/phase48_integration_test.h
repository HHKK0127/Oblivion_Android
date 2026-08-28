#pragma once

// Phase 48: Game Loop Integration Test
// Full game loop simulation: init -> update -> render -> cleanup
// Tests cell transitions, NPC lifecycle, combat, quests, save/load, and performance

#include <string>
#include <vector>
#include <cstdint>

struct Phase48TestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class Phase48IntegrationTest {
public:
    Phase48IntegrationTest();
    ~Phase48IntegrationTest();

    // Run all integration tests. Returns true if all pass.
    bool runAllTests();

    // Get results after runAllTests
    const std::vector<Phase48TestResult>& getResults() const { return results_; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<Phase48TestResult> results_;

    // Test groups
    void testGameLoopSimulation();
    void testCellTransition();
    void testNpcSpawnDespawn();
    void testCombatFlow();
    void testQuestFlow();
    void testSaveLoadRoundtrip();
    void testPerformanceBenchmark();

    // Helpers
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
    static float getTimeMs();
};
