#pragma once

// Asset path resolution tests (Phase 66 P20 follow-up).
//
// Interior cells rendered zero meshes even with the real Meshes BSA installed,
// because ESM records name models relative to their type folder
// ("Dungeons\Misc\TrigZone01.NIF") while the BSA stores them under it
// ("Meshes\Dungeons\Misc\TrigZone01.NIF"). These tests pin the expansion rules
// that bridge the two spellings.

#include <string>
#include <vector>

struct AssetPathTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class AssetPathTests {
public:
    bool runAllTests();
    const std::vector<AssetPathTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<AssetPathTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
