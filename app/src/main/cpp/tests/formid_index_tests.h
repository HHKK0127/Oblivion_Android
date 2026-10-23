#pragma once

#include <string>
#include <vector>

struct FormIDIndexTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

// Covers assets/formid_index.{h,cpp}: the translation of a plugin's FormIDs from
// its own MAST-relative index space into the engine's load-order space. Pure
// integer handling, so it needs no game data.
class FormIDIndexTests {
public:
    bool runAllTests();
    const std::vector<FormIDIndexTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<FormIDIndexTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
};
