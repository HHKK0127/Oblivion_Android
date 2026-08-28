#pragma once

// Phase 48: Stress Test
// Memory, concurrent tasks, entities, textures, and event bus stress tests

#include <string>
#include <vector>
#include <cstdint>

struct Phase48StressResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class Phase48StressTest {
public:
    Phase48StressTest();
    ~Phase48StressTest();

    // Run all stress tests. Returns true if all pass.
    bool runAllTests();

    // Get results after runAllTests
    const std::vector<Phase48StressResult>& getResults() const { return results_; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<Phase48StressResult> results_;

    // Test groups
    void testMemoryStress();
    void testConcurrentTaskStress();
    void testEntityStress();
    void testTextureStress();
    void testEventBusStress();

    // Helpers
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
    static float getTimeMs();
};
