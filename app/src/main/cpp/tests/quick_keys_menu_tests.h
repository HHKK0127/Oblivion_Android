#pragma once

#include <string>
#include <vector>

struct QuickKeysMenuTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class QuickKeysMenuTests {
public:
    bool runAllTests();
    const std::vector<QuickKeysMenuTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<QuickKeysMenuTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
};