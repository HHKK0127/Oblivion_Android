#pragma once

#include <string>
#include <vector>

struct MenuStackTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

class MenuStackTests {
public:
    bool runAllTests();
    const std::vector<MenuStackTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MenuStackTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
};