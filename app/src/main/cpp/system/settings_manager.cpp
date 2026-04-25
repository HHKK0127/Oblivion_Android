#include "settings_manager.h"
#include <fstream>
#include <sstream>
#include <cstdlib>

SettingsManager::SettingsManager()
    : debugModeEnabled(true), currentLanguage("ja") {
    LOGD("SettingsManager created");
}

SettingsManager::~SettingsManager() {
    cleanup();
}

bool SettingsManager::initialize() {
    LOGD("SettingsManager::initialize() called");

    // デフォルト設定をリセット
    resetToDefaults();

    // 保存された設定を読み込み
    loadSettings();

    LOGD("SettingsManager initialized - Debug: %s, Lang: %s",
         debugModeEnabled ? "ON" : "OFF", currentLanguage.c_str());

    return true;
}

void SettingsManager::setDebugMode(bool enabled) {
    debugModeEnabled = enabled;
    LOGD("Debug mode set to: %s", enabled ? "ON" : "OFF");
    saveSettings();
}

void SettingsManager::setLanguage(const std::string& lang) {
    if (lang == "ja" || lang == "en") {
        currentLanguage = lang;
        LOGD("Language set to: %s", lang.c_str());
        saveSettings();
    } else {
        LOGW("Invalid language: %s", lang.c_str());
    }
}

void SettingsManager::saveSettings() {
    std::string filePath = getSettingsFilePath();

    std::ofstream file(filePath);
    if (!file.is_open()) {
        LOGW("Failed to open settings file for writing: %s", filePath.c_str());
        return;
    }

    // シンプルなテキスト形式で設定を保存
    file << "DEBUG_MODE=" << (debugModeEnabled ? "1" : "0") << "\n";
    file << "LANGUAGE=" << currentLanguage << "\n";

    file.close();
    LOGD("Settings saved to: %s", filePath.c_str());
}

void SettingsManager::loadSettings() {
    std::string filePath = getSettingsFilePath();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOGD("Settings file not found, using defaults: %s", filePath.c_str());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "DEBUG_MODE") {
            debugModeEnabled = (value == "1");
        } else if (key == "LANGUAGE") {
            if (value == "ja" || value == "en") {
                currentLanguage = value;
            }
        }
    }

    file.close();
    LOGD("Settings loaded from: %s", filePath.c_str());
}

void SettingsManager::resetToDefaults() {
    debugModeEnabled = true;  // デバッグモードはデフォルトON
    currentLanguage = "ja";   // 言語はデフォルト日本語
    LOGD("Settings reset to defaults");
}

void SettingsManager::cleanup() {
    LOGD("SettingsManager cleaned up");
}

std::string SettingsManager::getSettingsFilePath() const {
    // /data/data/com.example.oblivion/files/settings.txt に保存
    // または /sdcard/Android/data/com.example.oblivion/files/settings.txt
    std::string path = "/data/data/com.example.oblivion/settings.txt";
    return path;
}
