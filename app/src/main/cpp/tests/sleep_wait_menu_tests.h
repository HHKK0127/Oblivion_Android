#pragma once

#include <string>
#include <vector>

struct SleepWaitMenuTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class SleepWaitMenuTests {
public:
    bool runAllTests();
    const std::vector<SleepWaitMenuTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<SleepWaitMenuTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
    static bool near(float a, float b, float eps = 1e-4f);
};