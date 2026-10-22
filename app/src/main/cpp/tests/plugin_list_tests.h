#pragma once

#include <string>
#include <vector>

struct PluginListTestResult {
    std::string testName;
    bool passed;
    std::string message;
};

// Covers assets/plugin_list.{h,cpp}: the plugins.txt grammar the launcher's Data
// Files screen and the engine's startup load both rely on. Pure string handling,
// so it needs no game data.
class PluginListTests {
public:
    bool runAllTests();
    const std::vector<PluginListTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<PluginListTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "");
};
