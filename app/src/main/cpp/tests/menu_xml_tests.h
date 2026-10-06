#pragma once

// Menu XML interpreter tests.
//
// Covers the two-stage pipeline in ui/menu_xml_interpreter.h:
//   buildMenuDef()  - parse a menu XML file (lazy trait expressions)
//   resolve()       - evaluate every trait against a runtime context
//
// The core cases are built around the real book_menu.xml expressions
// (the _pagenum accumulator, the mult operator, the x controller traits and
// the &xbox; entity style) so the evaluator semantics are pinned to what the
// original game's menu system actually does, not to a guessed model.

#include <string>
#include <vector>

struct MenuXmlTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class MenuXmlTests {
public:
    bool runAllTests();
    const std::vector<MenuXmlTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<MenuXmlTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
