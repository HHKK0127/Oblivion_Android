#pragma once

// XTEL (door intercept target) decode tests.
//
// Covers the data-driven door transitions increment (Step 2). A door-base
// REFR in Oblivion3.esm carries an XTEL subrecord of 28 bytes: a leading u32
// FormID that points at the *target door reference* (not the destination
// cell), followed by 24 bytes of pos/rot. The destination cell is resolved
// lazily after all references and cells are loaded, so the raw decode must
// retain the door reference FormID and the pos/rot.
//
// The suite builds a synthetic ESM image in memory (a cell-children GRUP with
// three REFR records) so it runs without the real game data:
//   ref 0x00001000  XTEL 28 bytes (target door 0x00001BAD), plus NAME+DATA
//   ref 0x00001001  no XTEL
//   ref 0x00001002  XTEL truncated to 20 bytes (must be rejected, not crash)

#include <string>
#include <vector>

struct XtelTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class XtelDecodeTests {
public:
    bool runAllTests();
    const std::vector<XtelTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<XtelTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};