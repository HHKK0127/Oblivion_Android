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
 * @brief fDifficultyDamageMultiplier (GMST slot 0xB37B88). The EXE ships 10.0,
 * but Oblivion.esm overlays 5.0 when it loads, so the shipped game runs at 5.0.
 */
constexpr float DIFFICULTY_DAMAGE_MULTIPLIER = 5.0f;

/**
 * @brief fGlobalTimeMultiplier (GMST slot 0xB06704, "fGlobalTimeMultiplier:General").
 *
 * The EXE registers 1.0. Its only real consumer is the game clock at 0x47D170,
 * which multiplies the elapsed seconds before folding them into the time of day
 * (0x47D249 `fmul [0xB06704]`). RE §54.3: it is NOT a frame-delta multiplier -
 * scaling dt would run every subsystem 30x too fast - and it is a different
 * mechanism from the TimeScale setting (config object index 0x3A, default 30.0),
 * which only drives the SkyWeatherSystem clock and the wait/rest fast-forward.
 */
constexpr float GLOBAL_TIME_MULTIPLIER = 1.0f;

/**
 * @brief fAIMinGreetingDistance (GMST slot 0x00B36AD8), default 150.0.
 *
 * The distance a spoken line can be heard over. Subtitles are emitted at
 * 0x0057ACC0 when the matching toggle is on AND the listener is within this
 * distance times 1 (0x4D92BB / 0x5F72A8) or times 2 (0x62A247 / 0x62AE3C),
 * further scaled by the subtitle distance multiplier [0xA30634] (RE §47.3).
 */
constexpr float AI_MIN_GREETING_DISTANCE = 150.0f;

/**
 * @brief fPerkLightArmorNoviceDamageMult / fPerkHeavyArmorNoviceDamageMult
 * (slots 0x00B374F0 / 0x00B374F8), both 1.5.
 *
 * RE §45.2: the Novice tier of an armor mastery perk scales the damage the
 * armored actor takes by 1.5.
 */
constexpr float ARMOR_PERK_NOVICE_DAMAGE_MULT = 1.5f;

/**
 * @brief fPerkHeavyArmorJourneymanDamageMult / fPerkLightArmorJourneymanDamageMult
 * (slots 0x00B37500 / 0x00B37508), both 0.5.
 *
 * RE §45.2: the Journeyman tier (and every tier above it) scales the damage the
 * armored actor takes by 0.5.
 */
constexpr float ARMOR_PERK_JOURNEYMAN_DAMAGE_MULT = 0.5f;

/**
 * @brief Oblivion armor mastery tier for a skill value: 0 Novice (0-24),
 * 1 Apprentice (25-49), 2 Journeyman (50-74), 3 Expert (75-99), 4 Master (100+).
 */
inline int armorMasteryRank(float skill) {
    if (skill <= 0.0f) return 0;
    const int rank = static_cast<int>(skill) / 25;
    return rank > 4 ? 4 : rank;
}

/**
 * @brief Damage multiplier an armored actor's armor mastery perk applies to
 * incoming spell / reflected damage (RE §45.2, confirmed by §54.1 to be absent
 * from the melee and bow path FUN_005FEBF0).
 *
 * FUN_005f3870 reads only four GMST slots and picks between them with
 *     rank >= 2 -> Journeyman 0.5 / rank == 1 -> 1.0 / rank == 0 -> Novice 1.5
 * The heavy and light trees hold identical values, so the armor type worn only
 * selects which skill feeds `rank` - it cannot change the result. `armorSkill`
 * is therefore the rank of the armor skill the actor actually uses.
 */
inline float armorPerkDamageMultiplier(float armorSkill) {
    const int rank = armorMasteryRank(armorSkill);
    if (rank >= 2) return ARMOR_PERK_JOURNEYMAN_DAMAGE_MULT;
    if (rank == 0) return ARMOR_PERK_NOVICE_DAMAGE_MULT;
    return 1.0f;
}

/**
 * @brief Convert the 0..100 difficulty slider into the multiplier applied to
 * damage the player receives.
 *
 * RE §50.1 (correcting §33.1.3 / §49.3). FUN_005E2560 keeps the slider
 * normalised to d in [-1,+1] (default 0, i.e. slider 50) and evaluates
 * x = d * fDifficultyDamageMultiplier, then
 *     f = d > 0 ? x + 1 : d < 0 ? 1 / (1 - x) : 1
 * Both sides are continuous at d = 0 and the extremes are exact inverses:
 * slider 100 receives x6 and slider 0 receives x1/6.
 *
 * The previous pow(6, (d - 50) / 50) approximation only agreed at the three
 * points d = 0/50/100; at slider 25 it returned 3.50x where the engine returns
 * 0.286x.
 */
float difficultyDamageMultiplier(int difficulty);

/**
 * @brief Multiplier applied to damage the player deals: the reciprocal of
 * difficultyDamageMultiplier(). FUN_005E2560 divides by f when the player is
 * the attacker (RE §50.1).
 */
float difficultyDealtDamageMultiplier(int difficulty);

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
    // bSaveOnInteriorExteriorSwitch. gameplay_menu.xml has no row for this one -
    // it is an Oblivion.ini setting in the original - so it is persisted but never
    // shown. It gates the extra autosave when a cell transition crosses the
    // interior/exterior boundary (RE §47.4).
    bool saveOnInteriorExteriorSwitch = true;
    // bDisableDynamicCrosshair (0x00B13238, default 0). The shipped default is
    // off. RE §47.1 lists it as the eighth GamePlay toggle; gameplay_menu.xml has
    // no row for it, so like bSaveOnInteriorExteriorSwitch it is an
    // Oblivion.ini-only setting: persisted, never shown.
    bool disableDynamicCrosshair = false;
};

/**
 * @brief Video options that outlive a run.
 *
 * The original stores the chosen resolution in a settings global and only reads
 * it back at start-up - which is why committing a new one raises the "restart
 * required" message and nothing changes immediately (RE §36.5). resolution_index
 * indexes system/display_modes.h.
 */
struct VideoSettings {
    int resolution_index = 0;
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
     * @brief Get the persisted video options
     */
    const VideoSettings& videoSettings() const { return video; }

    /**
     * @brief Store the video options and persist them
     */
    void setVideoSettings(const VideoSettings& settings);

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
    VideoSettings video;

    // File path
    std::string getSettingsFilePath() const;

    // Apply one "KEY=VALUE" line for the AUDIO_* / GAMEPLAY_* / VIDEO_* option keys.
    void loadOptionSetting(const std::string& key, const std::string& value);

    static constexpr const char* LOG_TAG = "SettingsManager";
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
    #define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
};
