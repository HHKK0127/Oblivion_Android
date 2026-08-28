#pragma once

// BGM Manager
// Manages background music playback with crossfade between tracks,
// area-based playlist management, fade in/out, loop points, and volume ducking.

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "BgmManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declaration
class AudioManager;

namespace audio {

/**
 * @brief BGM playback state
 */
enum class BgmState : uint8_t {
    STOPPED = 0,
    PLAYING,
    FADING_IN,
    FADING_OUT,
    CROSSFADING
};

/**
 * @brief BGM track descriptor
 */
struct BgmTrack {
    std::string key;            // Unique identifier (e.g. "music/oblivion_theme")
    std::string filePath;       // Asset path
    float baseVolume;           // Default volume (0.0 - 1.0)
    float loopStart;            // Loop start in seconds (-1 = no loop point)
    float loopEnd;              // Loop end in seconds (-1 = end of track)
    bool loop;                  // Whether to loop

    BgmTrack()
        : baseVolume(1.0f), loopStart(-1.0f), loopEnd(-1.0f), loop(false) {
    }
};

/**
 * @brief Area-based BGM playlist
 */
struct BgmPlaylist {
    std::string areaName;                       // Area identifier
    std::vector<std::string> trackKeys;         // Ordered list of BGM keys
    int currentTrackIndex;                      // Current position in playlist
    bool shuffle;                               // Randomize order

    BgmPlaylist() : currentTrackIndex(0), shuffle(false) {
    }
};

/**
 * @brief Volume ducking state
 * Reduces BGM volume during dialogue, combat, etc.
 */
struct DuckState {
    bool active;
    float duckVolume;       // Target volume while ducked (0.0 - 1.0)
    float restoreVolume;    // Volume to restore after duck
    float fadeSpeed;        // Speed of duck/restore transition

    DuckState()
        : active(false), duckVolume(0.2f), restoreVolume(1.0f), fadeSpeed(2.0f) {
    }
};

/**
 * @brief BGM Manager
 * Handles background music playback with crossfade, playlists, and ducking.
 * Integrates with AudioManager for actual playback.
 */
class BgmManager {
public:
    BgmManager();
    ~BgmManager();

    /**
     * @brief Initialize with AudioManager reference
     * @param manager Pointer to the main AudioManager
     */
    void initialize(AudioManager* manager);

    /**
     * @brief Update BGM state (call every frame)
     * @param deltaTime Frame time in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    // ========== Track Registration ==========

    /**
     * @brief Register a BGM track
     * @param track Track descriptor
     */
    void registerTrack(const BgmTrack& track);

    /**
     * @brief Register a BGM track with key and path
     * @param key Unique key
     * @param filePath Asset path
     * @param volume Base volume
     * @param loop Whether to loop
     */
    void registerTrack(const std::string& key, const std::string& filePath,
                       float volume = 1.0f, bool loop = true);

    /**
     * @brief Check if a track is registered
     */
    bool hasTrack(const std::string& key) const;

    // ========== Playback Control ==========

    /**
     * @brief Play a BGM track by key
     * @param key Track key
     * @param fadeIn Fade-in duration in seconds (0 = immediate)
     * @return true on success
     */
    bool play(const std::string& key, float fadeIn = 0.0f);

    /**
     * @brief Stop current BGM
     * @param fadeOut Fade-out duration in seconds (0 = immediate)
     */
    void stop(float fadeOut = 0.0f);

    /**
     * @brief Crossfade from current track to a new one
     * @param key New track key
     * @param duration Crossfade duration in seconds
     * @return true on success
     */
    bool crossfade(const std::string& key, float duration = 2.0f);

    /**
     * @brief Pause current BGM
     */
    void pause();

    /**
     * @brief Resume paused BGM
     */
    void resume();

    /**
     * @brief Check if BGM is currently playing
     */
    bool isPlaying() const { return state != BgmState::STOPPED; }

    /**
     * @brief Get current track key
     */
    const std::string& getCurrentTrack() const { return currentTrackKey; }

    // ========== Volume Control ==========

    /**
     * @brief Set BGM volume
     * @param volume 0.0 - 1.0
     */
    void setVolume(float volume);

    /**
     * @brief Get current BGM volume
     */
    float getVolume() const { return volume; }

    /**
     * @brief Start volume ducking (reduce BGM for dialogue/combat)
     * @param duckLevel Target volume while ducked (0.0 - 1.0)
     * @param speed Transition speed
     */
    void startDucking(float duckLevel = 0.2f, float speed = 2.0f);

    /**
     * @brief Stop volume ducking (restore normal volume)
     */
    void stopDucking();

    // ========== Playlist Management ==========

    /**
     * @brief Create an area-based playlist
     * @param playlist Playlist descriptor
     */
    void createPlaylist(const BgmPlaylist& playlist);

    /**
     * @brief Play the playlist for a given area
     * @param areaName Area identifier
     * @param fadeIn Fade-in duration
     * @return true on success
     */
    bool playArea(const std::string& areaName, float fadeIn = 1.0f);

    /**
     * @brief Advance to next track in current playlist
     * @param crossfadeDuration Crossfade time (0 = immediate switch)
     */
    void nextTrack(float crossfadeDuration = 2.0f);

    /**
     * @brief Get current area name
     */
    const std::string& getCurrentArea() const { return currentArea; }

private:
    AudioManager* audioManager;

    // Track registry
    std::unordered_map<std::string, BgmTrack> tracks;

    // Playlist registry
    std::unordered_map<std::string, BgmPlaylist> playlists;

    // Current playback state
    BgmState state;
    std::string currentTrackKey;
    std::string currentArea;
    float volume;
    float currentFadeValue;

    // Crossfade state
    std::string crossfadeTargetKey;
    float crossfadeDuration;
    float crossfadeTimer;
    float crossfadeOutStartVolume;

    // Ducking
    DuckState duckState;
    float unduckedVolume;

    // Internal methods
    void updateFadeIn(float deltaTime);
    void updateFadeOut(float deltaTime);
    void updateCrossfade(float deltaTime);
    void updateDucking(float deltaTime);
    void applyVolume();
};

} // namespace audio
