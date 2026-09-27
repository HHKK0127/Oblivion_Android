#pragma once

// Game state report tests (Phase 66 P16).
//
// The `gamestate` console command exists so device verification never again has
// to guess whether the world is being drawn. These tests pin the exact line
// format the logcat parser (and a human) relies on, including the interior case
// where `grid` must NOT be printed because interior cells have no grid.

#include <string>
#include <vector>

struct GameStateReportTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class GameStateReportTests {
public:
    bool runAllTests();
    const std::vector<GameStateReportTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<GameStateReportTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
