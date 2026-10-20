#pragma once

#include <string>
#include <vector>

struct ContainerMenuTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class ContainerMenuTests {
public:
    bool runAllTests();
    const std::vector<ContainerMenuTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<ContainerMenuTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
    static bool near(float a, float b, float eps = 1e-4f);
};