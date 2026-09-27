#pragma once

// NIF geometry merge tests (Phase 66 P20 follow-up).
//
// A NIF scene stores its model as several NiTriShape / NiTriStrips blocks, but
// AssetManager and WorldLoader both owned a single Mesh and built it from
// geometries[0], so every other block of the model was dropped. These tests pin
// the merge rules: all geometries contribute, indices are rebased onto the
// merged vertex array, missing attributes fall back to the Vertex default, and
// unusable geometry is skipped instead of producing out-of-range indices.

#include <string>
#include <vector>

struct MeshBuilderTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class MeshBuilderTests {
public:
    bool runAllTests();
    const std::vector<MeshBuilderTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MeshBuilderTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
