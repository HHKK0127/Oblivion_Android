#pragma once

// RACE ATTR decode tests.
//
// V-664-2: the RACE attribute values live in the 16-byte ATTR subrecord, which
// is 8 male u8 followed by 8 female u8 in AV order Strength, Intelligence,
// Willpower, Agility, Speed, Endurance, Personality, Luck. Measured against
// Oblivion.esm on 2026-11-27 (all 15 records are 16 bytes); UESP's per-race
// male/female attribute table matches every value.
//
// The suite feeds two real payloads (Imperial 0x00000907 and Argonian
// 0x00023FE9, copied byte for byte from Oblivion.esm) through the live decode
// path (parseFromMemory -> decodeRecord -> decodeRace) so the layout is pinned
// by values. A synthetic "Distinct" record (all 8 values differ per sex) pins
// the male/female split, and 15-byte / 17-byte / missing ATTR pin the guards.
// Fields placed AFTER ATTR (FULL/DESC/MNAM/SPLO/DATA) are asserted on the
// ATTR-bearing records so the ATTR consumption cannot silently move them.
//
// The health source is V-664-3 and stays out of this suite: RaceData::
// startingHealth is 0 until that lands.

#include <string>
#include <vector>

struct RaceTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class RaceDecodeTests {
public:
    bool runAllTests();
    const std::vector<RaceTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<RaceTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
};
