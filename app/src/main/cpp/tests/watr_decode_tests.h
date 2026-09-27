#pragma once

// WATR (water) decode tests.
//
// Covers the P8 fix: WATR records whose DATA subrecord is shorter than 55 bytes
// carry no colour block (Blood, CamoranLava, CamoranLava02 in Oblivion3.esm).
// They must fall back to the DefaultWater colours instead of rendering black.
//
// The suite builds a synthetic ESM image in memory so it runs without the real
// (non-redistributable) game data.

#include <string>
#include <vector>

struct WatrTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class WatrDecodeTests {
public:
    bool runAllTests();
    const std::vector<WatrTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<WatrTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};
