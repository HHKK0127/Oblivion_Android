#pragma once

// Interior cell tests.
//
// Covers the P15 fix: the normal world build deliberately skips interior cells
// (WorldManager::addCellFromESM returns nullptr for them), which left the
// renderer's interior sky/water/fog suppression paths unreachable. The
// `teleportinterior` console command registers an interior cell on demand via
// WorldManager::enterInteriorCell, so those paths can actually run.
//
// The suite drives WorldManager directly; it needs no game data.

#include <string>
#include <vector>

struct InteriorCellTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class InteriorCellTests {
public:
    bool runAllTests();
    const std::vector<InteriorCellTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<InteriorCellTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
