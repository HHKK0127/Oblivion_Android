#pragma once

// Menu fade machine tests.
//
// Covers ui/menu_fade_system.h, the Android port of the original MenuManager
// fade node list (RE_EXE_REPORT.md 48.3 / 49.6). The cases pin the semantics
// the RE report recovered: linear one-shot fades, the 0.001 s duration floor,
// replace-by-source (FUN_00584300 unlink-then-create), auto-removal at
// progress 1.0 and the per-menu steady-state alpha.

#include <string>
#include <vector>

struct MenuFadeTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class MenuFadeTests {
public:
    bool runAllTests();
    const std::vector<MenuFadeTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MenuFadeTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
