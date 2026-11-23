#pragma once

#include <string>
#include <vector>

// Host test suite for RaceSexPanel (chargen RaceSexMenu 21-page wizard).
// Canon: docs/chargen_ui_spec.md. Exercises the built element registry
// (21 pages / 118 items, runtime ids) and walks both transition tables.
class RaceSexPanelTests {
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
