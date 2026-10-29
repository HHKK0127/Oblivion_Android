#pragma once

// SettingsManager layering tests.
//
// The port's own settings.txt outranks Oblivion.ini, which outranks the shipped
// defaults baked into resetToDefaults(). Two behaviours make that ordering work,
// and both are pinned here:
//
//   - initialize() only lets Oblivion.ini answer for the option keys settings.txt
//     does not own, so a player who edits Oblivion.ini keeps seeing those values
//     on the next start.
//   - saveSettings() rewrites only the keys an in-game menu actually changed. If
//     it wrote all of them the moment one slider moved, settings.txt would own
//     every key and silently shadow the whole INI file.
//
// The suite drives a scratch settings.txt injected through the SettingsManager
// constructor, so it never reads or writes a device path and needs no assets.

#include <string>
#include <vector>

struct SettingsManagerTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class SettingsManagerTests {
public:
    bool runAllTests();
    const std::vector<SettingsManagerTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    std::vector<SettingsManagerTestResult> results;
    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
};
