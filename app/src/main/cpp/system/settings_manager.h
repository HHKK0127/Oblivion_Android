#pragma once

#include <set>
#include <string>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#undef LOGI

/**
 * @brief Audio channel volumes, one per AudioMenu slider (Oblivion_default.ini
 * [Audio] fDefault*Volume, mirrored in system/oblivion_ini_defaults.cpp). The
 * engine applies gain = value^2 * master.
 *
 * The five values here are the shipped ones verbatim: master 1.0000, voice
 * 0.8000, effects 0.8000, foot 0.8000, music 0.3000. Only music is quiet in the
 * original - the other four start loud.
 */
struct AudioVolumeSettings {
    float master = 1.0f;   // fDefaultMasterVolume 1.0000
    float voice = 0.8f;    // fDefaultVoiceVolume  0.8000
    float effect = 0.8f;   // fDefaultEffectsVolume 0.8000
    float foot = 0.8f;     // fDefaultFootVolume   0.8000
    float music = 0.3f;    // fDefaultMusicVolume  0.3000
};

/**
 * @brief fDifficultyDamageMultiplier (GMST slot 0xB37B88). The EXE ships 10.0,
 * but Oblivion.esm overlays 5.0 when it loads, so the shipped game runs at 5.0.
 */
constexpr float DIFFICULTY_DAMAGE_MULTIPLIER = 5.0f;

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
 *
 * Every default below is the shipped Oblivion_default.ini [GamePlay] value, with
 * the compiled-in registry answering for the keys the file omits
 * (`bSaveOnInteriorExteriorSwitch`). Note bGeneralSubtitles ships as 0: a fresh
 * installation shows dialogue subtitles but not the general ones.
 */
struct GameplaySettings {
    int difficulty = 50;            // iDifficultyLevel = 50
    bool generalSubtitles = false;  // bGeneralSubtitles = 0
    bool dialogueSubtitles = true;  // bDialogueSubtitles = 1
    bool crosshair = true;          // bCrossHair = 1
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
 *
 * Values layer the way the original layers them:
 *
 *   settings.txt (the port's own file, rewritten whenever an option changes)
 *     > Oblivion.ini (a file the player drops next to the game data, as on PC)
 *       > the shipped defaults baked into resetToDefaults()
 *
 * saveSettings() only rewrites the keys this file already owns, and initialize()
 * only fills the remaining ones from Oblivion.ini. The two together mean an INI
 * the player edits keeps answering for every option the in-game menus have never
 * touched, instead of being shadowed the first time a slider moves.
 */
class SettingsManager {
public:
    /**
     * @brief @param settingsFilePath Overrides where settings.txt is read and
     * written. The default (empty) is the real per-app path; host tests pass a
     * scratch file so they never touch a device path.
     */
    explicit SettingsManager(const std::string& settingsFilePath = std::string());
    ~SettingsManager();

    /**
     * @brief Folder holding the game data. Oblivion.ini is looked up here, the
     * same way plugins.txt is. Set it before initialize() to have the INI read.
     */
    void setDataPath(const std::string& path);

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
    std::string dataPath;

    // Keys settings.txt has supplied (or a setter has changed). saveSettings()
    // rewrites only these and initialize() leaves them out of the Oblivion.ini
    // pass, which is what gives the file its priority.
    std::set<std::string> ownedOptionKeys;

    // File path
    std::string getSettingsFilePath() const;

    // Non-empty only when a test injected a scratch settings.txt path.
    std::string settingsFilePathOverride;

    // Apply one "KEY=VALUE" line for the AUDIO_* / GAMEPLAY_* / VIDEO_* option keys.
    // Returns true when the key was recognised and its value accepted.
    bool loadOptionSetting(const std::string& key, const std::string& value);

    // Fill every option settings.txt did not own from <dataPath>/Oblivion.ini,
    // falling back to the merged default table for keys the file omits.
    void loadOblivionIni();

    static constexpr const char* LOG_TAG = "SettingsManager";
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
    #define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
};
