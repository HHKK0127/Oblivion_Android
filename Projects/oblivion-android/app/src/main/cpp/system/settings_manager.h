#pragma once

#include <string>
#include <unordered_map>

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    bool initialize();
    void cleanup();

    void setValue(const std::string& key, int value);
    void setValue(const std::string& key, float value);
    void setValue(const std::string& key, bool value);
    void setValue(const std::string& key, const std::string& value);

    int getIntValue(const std::string& key, int defaultValue = 0) const;
    float getFloatValue(const std::string& key, float defaultValue = 0.0f) const;
    bool getBoolValue(const std::string& key, bool defaultValue = false) const;
    std::string getStringValue(const std::string& key, const std::string& defaultValue = "") const;

    bool loadSettings();
    bool saveSettings();

private:
    std::unordered_map<std::string, std::string> settings;
    std::string settingsPath;
};