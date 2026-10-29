#include "settings_manager.h"
#include "oblivion_ini.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace {

enum class IniKind { kBool, kInt, kFloat };

// Oblivion.ini key -> the settings.txt option key it feeds. The option key is
// what decides whether the port already owns the value, so every entry here must
// have a matching branch in loadOptionSetting().
struct IniOptionMap {
    const char* section;
    const char* iniKey;
    const char* optionKey;
    IniKind kind;
};

const IniOptionMap kIniOptionMap[] = {
    {"Audio", "fDefaultMasterVolume", "AUDIO_MASTER", IniKind::kFloat},
    {"Audio", "fDefaultVoiceVolume", "AUDIO_VOICE", IniKind::kFloat},
    {"Audio", "fDefaultEffectsVolume", "AUDIO_EFFECT", IniKind::kFloat},
    {"Audio", "fDefaultFootVolume", "AUDIO_FOOT", IniKind::kFloat},
    {"Audio", "fDefaultMusicVolume", "AUDIO_MUSIC", IniKind::kFloat},
    {"GamePlay", "iDifficultyLevel", "GAMEPLAY_DIFFICULTY", IniKind::kInt},
    {"GamePlay", "bGeneralSubtitles", "GAMEPLAY_GENERAL_SUBTITLES", IniKind::kBool},
    {"GamePlay", "bDialogueSubtitles", "GAMEPLAY_DIALOGUE_SUBTITLES", IniKind::kBool},
    {"GamePlay", "bCrossHair", "GAMEPLAY_CROSSHAIR", IniKind::kBool},
    {"GamePlay", "bSaveOnRest", "GAMEPLAY_SAVE_ON_REST", IniKind::kBool},
    {"GamePlay", "bSaveOnWait", "GAMEPLAY_SAVE_ON_WAIT", IniKind::kBool},
    {"GamePlay", "bSaveOnTravel", "GAMEPLAY_SAVE_ON_TRAVEL", IniKind::kBool},
    {"GamePlay", "bSaveOnInteriorExteriorSwitch", "GAMEPLAY_SAVE_ON_INT_EXT_SWITCH",
     IniKind::kBool},
    {"GamePlay", "bDisableDynamicCrosshair", "GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR",
     IniKind::kBool},
};

}  // namespace

float difficultyDamageMultiplier(int difficulty) {
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 100) difficulty = 100;
    // The engine keeps the slider normalised to [-1,+1] with 0 at the centre.
    const float d = (static_cast<float>(difficulty) - 50.0f) / 50.0f;
    const float x = d * DIFFICULTY_DAMAGE_MULTIPLIER;
    if (d > 0.0f) return x + 1.0f;           // Above the default: take more damage.
    if (d < 0.0f) return 1.0f / (1.0f - x);  // Below the default: take less damage.
    return 1.0f;
}

float difficultyDealtDamageMultiplier(int difficulty) {
    return 1.0f / difficultyDamageMultiplier(difficulty);
}

SettingsManager::SettingsManager(const std::string& settingsFilePath)
    : debugModeEnabled(true), currentLanguage("en"), fpsCap(60),
      settingsFilePathOverride(settingsFilePath) {
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

    // Then let Oblivion.ini answer for whatever settings.txt does not own.
    loadOblivionIni();

    LOGD("SettingsManager initialized - Debug: %s, Lang: %s",
         debugModeEnabled ? "ON" : "OFF", currentLanguage.c_str());

    return true;
}

void SettingsManager::setDebugMode(bool enabled) {
    debugModeEnabled = enabled;
    ownedOptionKeys.insert("DEBUG_MODE");
    LOGD("Debug mode set to: %s", enabled ? "ON" : "OFF");
    saveSettings();
}

void SettingsManager::setLanguage(const std::string& lang) {
    if (lang == "ja" || lang == "en") {
        currentLanguage = lang;
        ownedOptionKeys.insert("LANGUAGE");
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
    ownedOptionKeys.insert("FPS_CAP");
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
    ownedOptionKeys.insert("AUDIO_MASTER");
    ownedOptionKeys.insert("AUDIO_VOICE");
    ownedOptionKeys.insert("AUDIO_EFFECT");
    ownedOptionKeys.insert("AUDIO_FOOT");
    ownedOptionKeys.insert("AUDIO_MUSIC");
    saveSettings();
}

void SettingsManager::setGameplaySettings(const GameplaySettings& settings) {
    gameplay = settings;
    if (gameplay.difficulty < 0) gameplay.difficulty = 0;
    if (gameplay.difficulty > 100) gameplay.difficulty = 100;
    ownedOptionKeys.insert("GAMEPLAY_DIFFICULTY");
    ownedOptionKeys.insert("GAMEPLAY_GENERAL_SUBTITLES");
    ownedOptionKeys.insert("GAMEPLAY_DIALOGUE_SUBTITLES");
    ownedOptionKeys.insert("GAMEPLAY_CROSSHAIR");
    ownedOptionKeys.insert("GAMEPLAY_SAVE_ON_REST");
    ownedOptionKeys.insert("GAMEPLAY_SAVE_ON_WAIT");
    ownedOptionKeys.insert("GAMEPLAY_SAVE_ON_TRAVEL");
    ownedOptionKeys.insert("GAMEPLAY_SAVE_ON_INT_EXT_SWITCH");
    ownedOptionKeys.insert("GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR");
    saveSettings();
}

void SettingsManager::setVideoSettings(const VideoSettings& settings) {
    video = settings;
    if (video.resolution_index < 0) video.resolution_index = 0;
    ownedOptionKeys.insert("VIDEO_RESOLUTION_INDEX");
    saveSettings();
}

void SettingsManager::setDataPath(const std::string& path) {
    dataPath = path;
}

void SettingsManager::saveSettings() {
    std::string filePath = getSettingsFilePath();

    std::ofstream file(filePath);
    if (!file.is_open()) {
        LOGW("Failed to open settings file for writing: %s", filePath.c_str());
        return;
    }

    // Rewrite only the keys this file owns. A key the player never changed stays
    // out of settings.txt so that Oblivion.ini - or the shipped default - keeps
    // answering for it.
    const auto owned = [this](const char* key) { return ownedOptionKeys.count(key) != 0; };

    if (owned("DEBUG_MODE")) {
        file << "DEBUG_MODE=" << (debugModeEnabled ? "1" : "0") << "\n";
    }
    if (owned("LANGUAGE")) {
        file << "LANGUAGE=" << currentLanguage << "\n";
    }
    if (owned("FPS_CAP")) {
        file << "FPS_CAP=" << fpsCap << "\n";
    }
    if (owned("AUDIO_MASTER")) file << "AUDIO_MASTER=" << audioSettings.master << "\n";
    if (owned("AUDIO_VOICE")) file << "AUDIO_VOICE=" << audioSettings.voice << "\n";
    if (owned("AUDIO_EFFECT")) file << "AUDIO_EFFECT=" << audioSettings.effect << "\n";
    if (owned("AUDIO_FOOT")) file << "AUDIO_FOOT=" << audioSettings.foot << "\n";
    if (owned("AUDIO_MUSIC")) file << "AUDIO_MUSIC=" << audioSettings.music << "\n";
    if (owned("GAMEPLAY_DIFFICULTY")) {
        file << "GAMEPLAY_DIFFICULTY=" << gameplay.difficulty << "\n";
    }
    if (owned("GAMEPLAY_GENERAL_SUBTITLES")) {
        file << "GAMEPLAY_GENERAL_SUBTITLES=" << (gameplay.generalSubtitles ? "1" : "0")
             << "\n";
    }
    if (owned("GAMEPLAY_DIALOGUE_SUBTITLES")) {
        file << "GAMEPLAY_DIALOGUE_SUBTITLES=" << (gameplay.dialogueSubtitles ? "1" : "0")
             << "\n";
    }
    if (owned("GAMEPLAY_CROSSHAIR")) {
        file << "GAMEPLAY_CROSSHAIR=" << (gameplay.crosshair ? "1" : "0") << "\n";
    }
    if (owned("GAMEPLAY_SAVE_ON_REST")) {
        file << "GAMEPLAY_SAVE_ON_REST=" << (gameplay.saveOnRest ? "1" : "0") << "\n";
    }
    if (owned("GAMEPLAY_SAVE_ON_WAIT")) {
        file << "GAMEPLAY_SAVE_ON_WAIT=" << (gameplay.saveOnWait ? "1" : "0") << "\n";
    }
    if (owned("GAMEPLAY_SAVE_ON_TRAVEL")) {
        file << "GAMEPLAY_SAVE_ON_TRAVEL=" << (gameplay.saveOnTravel ? "1" : "0") << "\n";
    }
    if (owned("GAMEPLAY_SAVE_ON_INT_EXT_SWITCH")) {
        file << "GAMEPLAY_SAVE_ON_INT_EXT_SWITCH="
             << (gameplay.saveOnInteriorExteriorSwitch ? "1" : "0") << "\n";
    }
    if (owned("GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR")) {
        file << "GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR="
             << (gameplay.disableDynamicCrosshair ? "1" : "0") << "\n";
    }
    if (owned("VIDEO_RESOLUTION_INDEX")) {
        file << "VIDEO_RESOLUTION_INDEX=" << video.resolution_index << "\n";
    }

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
            ownedOptionKeys.insert(key);
        } else if (key == "LANGUAGE") {
            if (value == "ja" || value == "en") {
                currentLanguage = value;
                ownedOptionKeys.insert(key);
            }
        } else if (key == "FPS_CAP") {
            try {
                int fps = std::stoi(value);
                if (fps >= 15 && fps <= 120) {
                    fpsCap = fps;
                    ownedOptionKeys.insert(key);
                }
            } catch (...) {
                LOGW("Invalid FPS_CAP value: %s", value.c_str());
            }
        } else if (key.rfind("AUDIO_", 0) == 0 || key.rfind("GAMEPLAY_", 0) == 0 ||
                   key.rfind("VIDEO_", 0) == 0) {
            if (loadOptionSetting(key, value)) {
                ownedOptionKeys.insert(key);
            }
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
    ownedOptionKeys.clear();
    LOGD("Settings reset to defaults");
}

bool SettingsManager::loadOptionSetting(const std::string& key, const std::string& value) {
    // Boolean option keys are stored as "0"/"1"; the rest are numeric.
    // Returns true only when the key is recognised and its value was accepted, so
    // a caller can tell an applied option from one that should keep falling
    // through to a lower layer.
    if (key == "GAMEPLAY_GENERAL_SUBTITLES") {
        gameplay.generalSubtitles = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_DIALOGUE_SUBTITLES") {
        gameplay.dialogueSubtitles = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_CROSSHAIR") {
        gameplay.crosshair = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_SAVE_ON_REST") {
        gameplay.saveOnRest = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_SAVE_ON_WAIT") {
        gameplay.saveOnWait = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_SAVE_ON_TRAVEL") {
        gameplay.saveOnTravel = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_SAVE_ON_INT_EXT_SWITCH") {
        gameplay.saveOnInteriorExteriorSwitch = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_DISABLE_DYNAMIC_CROSSHAIR") {
        gameplay.disableDynamicCrosshair = (value == "1");
        return true;
    }
    if (key == "GAMEPLAY_DIFFICULTY") {
        try {
            int level = std::stoi(value);
            if (level >= 0 && level <= 100) {
                gameplay.difficulty = level;
                return true;
            }
        } catch (...) {
            LOGW("Invalid GAMEPLAY_DIFFICULTY value: %s", value.c_str());
        }
        return false;
    }
    if (key == "VIDEO_RESOLUTION_INDEX") {
        try {
            int index = std::stoi(value);
            // The list is rebuilt from the platform at every start (RE §36), so a
            // saved index is clamped again when VideoOptionsUI opens.
            if (index >= 0) {
                video.resolution_index = index;
                return true;
            }
        } catch (...) {
            LOGW("Invalid VIDEO_RESOLUTION_INDEX value: %s", value.c_str());
        }
        return false;
    }

    float* target = nullptr;
    if (key == "AUDIO_MASTER") target = &audioSettings.master;
    else if (key == "AUDIO_VOICE") target = &audioSettings.voice;
    else if (key == "AUDIO_EFFECT") target = &audioSettings.effect;
    else if (key == "AUDIO_FOOT") target = &audioSettings.foot;
    else if (key == "AUDIO_MUSIC") target = &audioSettings.music;
    if (!target) {
        return false;
    }
    try {
        float parsed = std::stof(value);
        if (parsed >= 0.0f && parsed <= 1.0f) {
            *target = parsed;
            return true;
        }
    } catch (...) {
        LOGW("Invalid %s value: %s", key.c_str(), value.c_str());
    }
    return false;
}

void SettingsManager::loadOblivionIni() {
    if (dataPath.empty()) {
        LOGD("Settings: no data path set, Oblivion.ini skipped");
        return;
    }

    const std::string iniPath = dataPath + "/Oblivion.ini";
    oblivion::OblivionIni ini;
    if (!ini.load_file(iniPath)) {
        // Every lookup would fall back to the merged default table, which already
        // matches the values resetToDefaults() installed, so there is nothing to do.
        LOGD("Settings: %s not readable, using built-in defaults", iniPath.c_str());
        return;
    }

    int applied = 0;
    for (const IniOptionMap& entry : kIniOptionMap) {
        // Only the file itself counts here. A key answered by the default table is
        // already identical to the shipped default, and copying it in would hide
        // which layer really decided.
        if (ini.source_of(entry.section, entry.iniKey) !=
            oblivion::OblivionIni::ValueSource::kFile) {
            continue;
        }
        if (ownedOptionKeys.count(entry.optionKey) != 0) {
            continue;  // settings.txt owns the option and keeps priority.
        }

        std::string text;
        switch (entry.kind) {
            case IniKind::kBool:
                text = ini.get_bool(entry.section, entry.iniKey) ? "1" : "0";
                break;
            case IniKind::kInt:
                text = std::to_string(ini.get_int(entry.section, entry.iniKey));
                break;
            case IniKind::kFloat: {
                std::ostringstream out;
                out << ini.get_float(entry.section, entry.iniKey);
                text = out.str();
                break;
            }
        }

        if (loadOptionSetting(entry.optionKey, text)) {
            // Deliberately not added to ownedOptionKeys: the INI keeps governing the
            // option until the player changes it from a menu, which is what makes
            // editing Oblivion.ini effective on the next start.
            ++applied;
        } else {
            LOGW("Settings: Oblivion.ini %s ignored (%s=%s)", entry.optionKey, entry.iniKey,
                 text.c_str());
        }
    }

    LOGD("Settings: Oblivion.ini applied %d option(s) from %s", applied, iniPath.c_str());
}

void SettingsManager::cleanup() {
    LOGD("SettingsManager cleaned up");
}

std::string SettingsManager::getSettingsFilePath() const {
    if (!settingsFilePathOverride.empty()) {
        return settingsFilePathOverride;
    }
    // Save to /data/data/com.example.oblivion/files/settings.txt
    // Or /sdcard/Android/data/com.example.oblivion/files/settings.txt
    std::string path = "/data/data/com.example.oblivion/settings.txt";
    return path;
}
