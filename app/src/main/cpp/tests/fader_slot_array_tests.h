#pragma once

// Fader slot array tests.
//
// Covers ui/fader_slot_array.h, the Android port of the original 3-slot
// full-screen fader array at 0x00B12DC0 (RE_EXE_REPORT.md 48.2, raw
// disassembly tmp/re/f572f60.txt). The cases pin the .data initials, the
// FUN_00572EC0 reset paths (including the errata: +0x00/+0x04 survive a
// reset) and the FUN_00572F60 ramp-up / ramp-down / release semantics.

#include <string>
#include <vector>

struct FaderSlotArrayTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class FaderSlotArrayTests {
public:
    bool runAllTests();
    const std::vector<FaderSlotArrayTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<FaderSlotArrayTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
