#include "settings_manager.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>

float difficultyDamageMultiplier(int difficulty) {
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 100) difficulty = 100;
    // 0 -> 1/6, 50 -> 1.0, 100 -> 6.0 (exponential in the slider offset).
    const float exponent = (static_cast<float>(difficulty) - 50.0f) / 50.0f;
    return std::pow(6.0f, exponent);
}

SettingsManager::SettingsManager()
    : debugModeEnabled(true), currentLanguage("en"), fpsCap(60) {
    LOGD("SettingsManager created");
}

SettingsManager::~SettingsManager() {
    cleanup();
}

bool SettingsManager::initialize() {
    LOGD("SettingsManager::initialize() called");

    // Reset to default settings
    resetToDefaults();

    // Load saved settings
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

void SettingsManager::setFpsCap(int fps) {
    if (fps < 15 || fps > 120) {
        LOGW("Invalid FPS cap: %d (expected 15-120)", fps);
        return;
    }
    fpsCap = fps;
    LOGD("FPS cap set to: %d", fps);
    saveSettings();
}

void SettingsManager::setAudioVolumes(const AudioVolumeSettings& volumes) {
    audioSettings = volumes;
    // Keep the sliders inside the range the original accepts (0..1).
    float* const values[] = {&audioSettings.master, &audioSettings.voice,
                             &audioSettings.effect, &audioSettings.foot,
                             &audioSettings.music};
    for (float* const value : values) {
        if (*value < 0.0f) *value = 0.0f;
        if (*value > 1.0f) *value = 1.0f;
    }
    saveSettings();
}

void SettingsManager::setGameplaySettings(const GameplaySettings& settings) {
    gameplay = settings;
    if (gameplay.difficulty < 0) gameplay.difficulty = 0;
    if (gameplay.difficulty > 100) gameplay.difficulty = 100;
    saveSettings();
}

void SettingsManager::setVideoSettings(const VideoSettings& settings) {
    video = settings;
    if (video.resolution_index < 0) video.resolution_index = 0;
    saveSettings();
}

void SettingsManager::saveSettings() {
    std::string filePath = getSettingsFilePath();

    std::ofstream file(filePath);
    if (!file.is_open()) {
        LOGW("Failed to open settings file for writing: %s", filePath.c_str());
        return;
    }

    // Save settings in simple text format
    file << "DEBUG_MODE=" << (debugModeEnabled ? "1" : "0") << "\n";
    file << "LANGUAGE=" << currentLanguage << "\n";
    file << "FPS_CAP=" << fpsCap << "\n";
    file << "AUDIO_MASTER=" << audioSettings.master << "\n";
    file << "AUDIO_VOICE=" << audioSettings.voice << "\n";
    file << "AUDIO_EFFECT=" << audioSettings.effect << "\n";
    file << "AUDIO_FOOT=" << audioSettings.foot << "\n";
    file << "AUDIO_MUSIC=" << audioSettings.music << "\n";
    file << "GAMEPLAY_DIFFICULTY=" << gameplay.difficulty << "\n";
    file << "GAMEPLAY_GENERAL_SUBTITLES=" << (gameplay.generalSubtitles ? "1" : "0") << "\n";
    file << "GAMEPLAY_DIALOGUE_SUBTITLES=" << (gameplay.dialogueSubtitles ? "1" : "0") << "\n";
    file << "GAMEPLAY_CROSSHAIR=" << (gameplay.crosshair ? "1" : "0") << "\n";
    file << "GAMEPLAY_SAVE_ON_REST=" << (gameplay.saveOnRest ? "1" : "0") << "\n";
    file << "GAMEPLAY_SAVE_ON_WAIT=" << (gameplay.saveOnWait ? "1" : "0") << "\n";
    file << "GAMEPLAY_SAVE_ON_TRAVEL=" << (gameplay.saveOnTravel ? "1" : "0") << "\n";
    file << "GAMEPLAY_SAVE_ON_INT_EXT_SWITCH="
         << (gameplay.saveOnInteriorExteriorSwitch ? "1" : "0") << "\n";
    file << "VIDEO_RESOLUTION_INDEX=" << video.resolution_index << "\n";

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
        } else if (key == "FPS_CAP") {
            try {
                int fps = std::stoi(value);
                if (fps >= 15 && fps <= 120) {
                    fpsCap = fps;
                }
            } catch (...) {
                LOGW("Invalid FPS_CAP value: %s", value.c_str());
            }
        } else if (key.rfind("AUDIO_", 0) == 0 || key.rfind("GAMEPLAY_", 0) == 0 ||
                   key.rfind("VIDEO_", 0) == 0) {
            loadOptionSetting(key, value);
        }
    }

    file.close();
    LOGD("Settings loaded from: %s", filePath.c_str());
}

void SettingsManager::resetToDefaults() {
    debugModeEnabled = true;  // Debug mode is ON by default
    currentLanguage = "en";   // Language defaults to English (Japanese rendering not yet supported)
    fpsCap = 60;              // 60 fps is the default frame rate cap
    audioSettings = AudioVolumeSettings{};
    gameplay = GameplaySettings{};
    video = VideoSettings{};
    LOGD("Settings reset to defaults");
}

void SettingsManager::loadOptionSetting(const std::string& key, const std::string& value) {
    // Boolean option keys are stored as "0"/"1"; the rest are numeric.
    if (key == "GAMEPLAY_GENERAL_SUBTITLES") {
        gameplay.generalSubtitles = (value == "1");
    } else if (key == "GAMEPLAY_DIALOGUE_SUBTITLES") {
        gameplay.dialogueSubtitles = (value == "1");
    } else if (key == "GAMEPLAY_CROSSHAIR") {
        gameplay.crosshair = (value == "1");
    } else if (key == "GAMEPLAY_SAVE_ON_REST") {
        gameplay.saveOnRest = (value == "1");
    } else if (key == "GAMEPLAY_SAVE_ON_WAIT") {
        gameplay.saveOnWait = (value == "1");
    } else if (key == "GAMEPLAY_SAVE_ON_TRAVEL") {
        gameplay.saveOnTravel = (value == "1");
    } else if (key == "GAMEPLAY_SAVE_ON_INT_EXT_SWITCH") {
        gameplay.saveOnInteriorExteriorSwitch = (value == "1");
    } else if (key == "GAMEPLAY_DIFFICULTY") {
        try {
            int level = std::stoi(value);
            if (level >= 0 && level <= 100) {
                gameplay.difficulty = level;
            }
        } catch (...) {
            LOGW("Invalid GAMEPLAY_DIFFICULTY value: %s", value.c_str());
        }
    } else if (key == "VIDEO_RESOLUTION_INDEX") {
        try {
            int index = std::stoi(value);
            // The list is rebuilt from the platform at every start (RE §36), so a
            // saved index is clamped again when VideoOptionsUI opens.
            if (index >= 0) {
                video.resolution_index = index;
            }
        } catch (...) {
            LOGW("Invalid VIDEO_RESOLUTION_INDEX value: %s", value.c_str());
        }
    } else {
        float* target = nullptr;
        if (key == "AUDIO_MASTER") target = &audioSettings.master;
        else if (key == "AUDIO_VOICE") target = &audioSettings.voice;
        else if (key == "AUDIO_EFFECT") target = &audioSettings.effect;
        else if (key == "AUDIO_FOOT") target = &audioSettings.foot;
        else if (key == "AUDIO_MUSIC") target = &audioSettings.music;
        if (!target) {
            return;
        }
        try {
            float parsed = std::stof(value);
            if (parsed >= 0.0f && parsed <= 1.0f) {
                *target = parsed;
            }
        } catch (...) {
            LOGW("Invalid %s value: %s", key.c_str(), value.c_str());
        }
    }
}

void SettingsManager::cleanup() {
    LOGD("SettingsManager cleaned up");
}

std::string SettingsManager::getSettingsFilePath() const {
    // Save to /data/data/com.example.oblivion/files/settings.txt
    // Or /sdcard/Android/data/com.example.oblivion/files/settings.txt
    std::string path = "/data/data/com.example.oblivion/settings.txt";
    return path;
}
