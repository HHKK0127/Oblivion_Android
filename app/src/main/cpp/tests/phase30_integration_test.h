#pragma once

// Phase 30 Step 13: Integration Test
// Tests the full NIF pipeline: parse -> skeleton -> skinning -> animation -> collision
// Runs on-device via JNI with real Oblivion NIF data

#include <string>
#include <vector>

struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class Phase30IntegrationTest {
public:
    Phase30IntegrationTest();
    ~Phase30IntegrationTest();

    // Run all tests. Returns true if all pass.
    bool runAllTests(const std::string& assetBasePath);

    // Get results after runAllTests
    const std::vector<TestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<TestResult> results;
    std::string basePath;

    // Test groups
    void testNIFParsing();
    void testSkeletonBuilding();
    void testSkinPartitionPacking();
    void testAnimationParsing();
    void testAnimationPlayback();
    void testCollisionParsing();
    void testCollisionWorld();
    void testCharacterController();
    void testFullPipeline();

    // Helpers
    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);
    std::vector<std::string> findNIFFiles(const std::string& directory) const;
    bool fileExists(const std::string& path) const;
};
