#pragma once

// Oblivion.ini reader tests.
//
// Two things are pinned here. First, the authenticity of the merged default
// table: the exact entry count, the type and provenance breakdown, and - the
// trap that once broke the binary search - that the table is really ordered by
// (lowercase section, lowercase key), because the shipped file spells a section
// [HAVOK] while the compiled-in registry spells it [Havok].
//
// Second, the parser and resolver semantics the original engine has: the first
// occurrence of a duplicate key wins (its registry uses find-or-insert), only
// whole-line comments are skipped, an empty value is legal, and a key the file
// does not mention falls back to the compiled-in default.

#include <string>
#include <vector>

struct OblivionIniTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class OblivionIniTests {
public:
    bool runAllTests();
    const std::vector<OblivionIniTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<OblivionIniTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
