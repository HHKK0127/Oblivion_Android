#pragma once

// LIP file decode tests.
//
// Validates the reverse-engineered .lip format (Phase 65). Tests build
// synthetic lip payloads in memory (both the standard token-stream form and
// the decode-reject cases) so the suite runs without the real game data:
//   - standard file: header + token stream expanding to count*132 keyframes
//   - 0xFF-prefixed variant: must be rejected (lip disabled)
//   - empty / undersized: must be rejected
//   - malformed token stream: truncated length field, expired max size
//   - round-trip fidelity of the 16 viseme + 17 aux floats per frame

#include <string>
#include <vector>

struct LipTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class LipSyncDecodeTests {
public:
    bool runAllTests();
    const std::vector<LipTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<LipTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};