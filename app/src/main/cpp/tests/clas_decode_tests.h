#pragma once

// CLAS (class) DATA decode tests.
//
// v261_clas_service_flags.md pins the on-disk layout of the 0x34-byte DATA
// subrecord (form+0x38 = DATA+0x00):
//   +0x00 u32 primary attribute 1 (attribute enum 0..7)
//   +0x04 u32 primary attribute 2 (0..7, must differ from attribute 1)
//   +0x08 u32 unread by the original engine
//   +0x0C 7 x u32 class skills, stored as skill ids 0x0C..0x20
//   +0x28 u8  flags: bit0 autocalcs, bit1 guard
//   +0x2C u32 unread by the original engine
//   +0x30 u8  trainer skill index (skill id = value + 0x0C)
//
// decodeClass() used to read a u32 flag word from +0x00 into a uint8_t field
// and a "specialization" byte from +0x04, i.e. it wrote four bytes into a
// one-byte member (three bytes past the object) and mislabelled attribute 2.
// The suite feeds two real Oblivion.esm payloads (Warrior 52 bytes, Priest
// 48 bytes) through the live decode path (parseFromMemory -> decodeRecord ->
// decodeClass) so the layout is pinned by values rather than by a compiler
// warning, and adds three synthetic records for the guard paths (a truncated
// DATA, a missing DATA and an out-of-window skill id). It runs under
// AddressSanitizer in the MSVC host runner so the overflow cannot come back
// unnoticed.

#include <string>
#include <vector>

struct ClasTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class ClasDecodeTests {
public:
    bool runAllTests();
    const std::vector<ClasTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<ClasTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};
