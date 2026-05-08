#include "settings_manager.h"
#include <android/log.h>
#include <sstream>

#define LOG_TAG_SETTINGS_MGR "SettingsManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SETTINGS_MGR, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SETTINGS_MGR, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_SETTINGS_MGR, __VA_ARGS__)

SettingsManager::SettingsManager()
    : settingsPath("/data/data/com.example.oblivion/files/settings.json") {
}

SettingsManager::~SettingsManager() {
    cleanup();
}

bool SettingsManager::initialize() {
    LOGI("SettingsManager initialized");
    return true;
}

void SettingsManager::cleanup() {
}

void SettingsManager::setValue(const std::string& key, int value) {
    settings[key] = std::to_string(value);
}

void SettingsManager::setValue(const std::string& key, float value) {
    settings[key] = std::to_string(value);
}

void SettingsManager::setValue(const std::string& key, bool value) {
    settings[key] = value ? "1" : "0";
}

void SettingsManager::setValue(const std::string& key, const std::string& value) {
    settings[key] = value;
}

int SettingsManager::getIntValue(const std::string& key, int defaultValue) const {
    auto it = settings.find(key);
    if (it == settings.end()) return defaultValue;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}

float SettingsManager::getFloatValue(const std::string& key, float defaultValue) const {
    auto it = settings.find(key);
    if (it == settings.end()) return defaultValue;
    try {
        return std::stof(it->second);
    } catch (...) {
        return defaultValue;
    }
}

bool SettingsManager::getBoolValue(const std::string& key, bool defaultValue) const {
    auto it = settings.find(key);
    if (it == settings.end()) return defaultValue;
    return it->second == "1" || it->second == "true" || it->second == "True";
}

std::string SettingsManager::getStringValue(const std::string& key, const std::string& defaultValue) const {
    auto it = settings.find(key);
    if (it == settings.end()) return defaultValue;
    return it->second;
}

bool SettingsManager::loadSettings() {
    LOGI("Loading settings from: %s", settingsPath.c_str());
    return true;
}

bool SettingsManager::saveSettings() {
    LOGI("Saving settings to: %s", settingsPath.c_str());
    return true;
}