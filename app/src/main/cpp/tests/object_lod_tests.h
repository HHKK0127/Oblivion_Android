#pragma once

// Phase 68: invariant tests for the shipped object LOD cells.
//
// The suite feeds synthetic byte buffers through the same object_lod_rules.h
// helpers the production provider uses, so a change to the record layout,
// the EOF-walk policy (RE report v255 §1, porting ruling §54.8 item 2) or
// the "<worldspace>_<x>_<y>.lod" naming grammar fails here instead of
// silently mis-placing statics on device.
//
// Everything is synthetic: no Oblivion assets are required, so there is no
// skip path.

#include <cstdint>
#include <string>
#include <vector>

struct ObjectLodTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class ObjectLodTests {
public:
    // Returns true if all tests pass.
    bool runAllTests();

    // Valid after runAllTests().
    const std::vector<ObjectLodTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void testRejectsUndersizedBuffer();
    void testRecordLayoutRoundTrip();
    void testEofWalkAdoptsTrailingRecords();
    void testPartialTailIsTrailingBytes();
    void testInstanceCountBounds();
    void testMalformedTailStopsWalk();
    void testMultiRecordFile();
    void testCellNameGrammar();
    void testCellNameRejections();

    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);

    std::vector<ObjectLodTestResult> results;
};
