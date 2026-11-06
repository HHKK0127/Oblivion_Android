#ifndef TESTS_DDS_UPLOAD_TESTS_H
#define TESTS_DDS_UPLOAD_TESTS_H

#include <string>
#include <vector>

struct DdsUploadTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

// Host tests for the compressed DDS upload path.
class DdsUploadTests {
public:
    DdsUploadTests() = default;

    bool runAllTests();
    const std::vector<DdsUploadTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void testLoadSyntheticDxt1();
    void testCompressedUploadFallsBackWithoutS3tc();
    void testCleanupIsIdempotent();
    void record(const std::string& name, bool passed,
                const std::string& msg = "", float ms = 0.0f);

    std::vector<DdsUploadTestResult> results;
};

#endif  // TESTS_DDS_UPLOAD_TESTS_H
