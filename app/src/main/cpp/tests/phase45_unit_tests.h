#pragma once

// Phase 45: Unit Tests
// Tests key subsystems: CombatManager, SpellManager, NPC, QuestManager,
// SaveManager, MemoryPool, AsyncTaskManager, CacheManager
// Runs on-device via JNI

#include <string>
#include <vector>

struct Phase45TestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class Phase45UnitTests {
public:
    Phase45UnitTests();
    ~Phase45UnitTests();

    // Run all tests. Returns true if all pass.
    bool runAllTests();

    // Get results after runAllTests
    const std::vector<Phase45TestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<Phase45TestResult> results;

    // Test groups (8 groups, 3-5 tests each)
    void testCombatManager();
    void testSpellManager();
    void testNPC();
    void testQuestManager();
    void testSaveManager();
    void testMemoryPool();
    void testAsyncTaskManager();
    void testCacheManager();

    // Helpers
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};
