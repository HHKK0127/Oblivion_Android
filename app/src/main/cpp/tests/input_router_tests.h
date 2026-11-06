#pragma once

#include <string>
#include <vector>

class InputRouterTests {
public:
    bool runAllTests();
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    struct Result {
        std::string name;
        bool passed;
        std::string message;
    };

    std::vector<Result> results;

    void record(const std::string& name, bool passed,
                const std::string& message = {});
};
