#pragma once

// Phase 69 persistence tests (containers).
//
// Containers are materialised lazily the first time the player opens one, so a
// load cannot restore them directly: SaveManager buffers the saved contents and
// hands them over through applyPendingContainerState() when the container is
// finally created. These tests pin that contract - including the carry-forward
// case where a save happens before the player ever opened the container - so a
// regression shows up on the host instead of as silent item loss in game.

#include <string>
#include <vector>

struct Phase69PersistenceTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class Phase69PersistenceTests {
public:
    bool runAllTests();
    const std::vector<Phase69PersistenceTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<Phase69PersistenceTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
