#pragma once

#include <string>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#undef LOGI

/**
 * @brief Audio channel volumes, one per AudioMenu slider (Oblivion_default.ini
 * [Audio] fDefault*Volume). The engine applies gain = value^2 * master.
 */
struct AudioVolumeSettings {
    float master = 0.8f;   // fDefaultMasterVolume
    float voice = 0.0f;    // fDefaultVoiceVolume
    float effect = 0.8f;   // fDefaultEffectsVolume
    float foot = 0.3f;     // fDefaultFootVolume
    float music = 0.8f;    // fDefaultMusicVolume
};

/**
 * @brief Convert the 0..100 difficulty slider into the multiplier applied to
 * damage the player receives.
 *
 * RE §30.2.3 confirms the slider value (fDifficultyMinValue 0.0 ..
 * fDifficultyMaxValue 100.0, default 50.0) is handed to the combat calculation
 * as-is, but the exact arithmetic inside the original combat routine is not yet
 * documented. The curve below reproduces Oblivion's observable behaviour: at the
 * default 50 the multiplier is 1.0, and it grows/shrinks exponentially so the
 * extremes sit a factor of six away from neutral.
 *
 * TODO(re): replace with the exact expression once the combat routine is read.
 */
float difficultyDamageMultiplier(int difficulty);

/**
 * @brief Gameplay options: difficulty slider plus the six On/Off toggles.
 */
struct GameplaySettings {
    int difficulty = 50;            // fDifficultyMinValue..MaxValue = 0..100
    bool generalSubtitles = true;   // bGeneralSubtitles
    bool dialogueSubtitles = true;  // bDialogueSubtitles
    bool crosshair = true;          // bCrossHair
    bool saveOnRest = true;         // bSaveOnRest
    bool saveOnWait = true;         // bSaveOnWait
    bool saveOnTravel = true;       // bSaveOnTravel
};

/**
 * @brief Game settings management system
 * Manages various game settings (language, debug mode, etc.)
 */
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    /**
     * @brief Initialize settings
     */
    bool initialize();

    /**
     * @brief Enable/disable debug mode
     */
    void setDebugMode(bool enabled);

    /**
     * @brief Check if debug mode is enabled
     */
    bool isDebugModeEnabled() const { return debugModeEnabled; }

    /**
     * @brief Set language ("ja" = Japanese, "en" = English)
     */
    void setLanguage(const std::string& lang);

    /**
     * @brief Get current language
     */
    std::string getLanguage() const { return currentLanguage; }

    /**
     * @brief Set the frame rate cap (15-120). Persisted with the other settings.
     */
    void setFpsCap(int fps);

    /**
     * @brief Get the frame rate cap
     */
    int getFpsCap() const { return fpsCap; }

    /**
     * @brief Get the persisted audio channel volumes
     */
    const AudioVolumeSettings& audioVolumes() const { return audioSettings; }

    /**
     * @brief Store the audio channel volumes and persist them
     */
    void setAudioVolumes(const AudioVolumeSettings& volumes);

    /**
     * @brief Get the persisted gameplay options
     */
    const GameplaySettings& gameplaySettings() const { return gameplay; }

    /**
     * @brief Store the gameplay options and persist them
     */
    void setGameplaySettings(const GameplaySettings& settings);

    /**
     * @brief Save settings
     */
    void saveSettings();

    /**
     * @brief Load settings
     */
    void loadSettings();

    /**
     * @brief Reset settings
     */
    void resetToDefaults();

    /**
     * @brief Cleanup
     */
    void cleanup();

private:
    bool debugModeEnabled;
    std::string currentLanguage;
    int fpsCap;
    AudioVolumeSettings audioSettings;
    GameplaySettings gameplay;

    // File path
    std::string getSettingsFilePath() const;

    // Apply one "KEY=VALUE" line for the AUDIO_* / GAMEPLAY_* option keys.
    void loadOptionSetting(const std::string& key, const std::string& value);

    static constexpr const char* LOG_TAG = "SettingsManager";
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
    #define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
};
