#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <android/log.h>

#ifdef AUDIO_SYSTEM_ENABLED
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "audio_clip.h"
#include "audio_source.h"
#include "audio_3d.h"

// JNI audio bridge extern declarations
extern "C" {
void jni_audio_call_play_bgm(const char* path);
void jni_audio_call_stop_bgm();
void jni_audio_call_play_se(const char* path);
}

// Forward declaration
class AudioManager;

// Global AudioManager pointer for UI and game systems
extern AudioManager* g_audioManager;

/**
 * @brief Audio manager
 * OpenAL device/context management,
 * centralized BGM/SE loading and playback control
 *
 * Follows Manager Pattern:
 * - initialize(): OpenAL initialization, device detection
 * - update(deltaTime): playback end check, 3D update
 * - cleanup(): resource release, device destruction
 */
class AudioManager {
public:
    /**
     * @brief Constructor
     */
    AudioManager();

    /**
     * @brief Destructor
     */
    ~AudioManager();

    // ========== Lifecycle ==========

    /**
     * @brief Initialize (OpenAL device/context)
     * @return True on success
     */
    bool initialize();

    /**
     * @brief Per-frame update (playback end check, 3D update)
     * @param deltaTime Frame time (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Cleanup (release all resources)
     */
    void cleanup();

    // ========== Clip Management ==========

    /**
     * @brief Load audio clip
     * @param filename WAV file path (e.g. "assets/music/oblivion_theme.wav")
     * @param type 0=BGM, 1=SE, 2=Voice
     * @param isLooping Loop playback flag
     * @return Clip ID (0 on failure)
     */
    uint32_t loadClip(const std::string& filename, uint8_t type = 1,
                     bool isLooping = false);

    /**
     * @brief Get clip from resources
     * @param clipId Clip ID
     * @return Clip pointer (nullptr if not found)
     */
    AudioClip* getClip(uint32_t clipId);

    /**
     * @brief Unload clip
     * @param clipId Clip ID
     */
    void unloadClip(uint32_t clipId);

    // ========== BGM Management ==========

    /**
     * @brief Play BGM
     * @param clipId Loaded clip ID
     * @param fadeIn Fade in time (seconds, 0 = immediate)
     * @return True on success
     */
    bool playBGM(uint32_t clipId, float fadeIn = 0.0f);

    /**
     * @brief Stop BGM
     * @param fadeOut Fade out time (seconds, 0 = immediate)
     */
    void stopBGM(float fadeOut = 0.0f);

    /**
     * @brief Is BGM playing
     */
    bool isBGMPlaying() const;

    /**
     * @brief Set BGM volume
     * @param volume 0.0 - 1.0
     */
    void setBGMVolume(float volume);

    /**
     * @brief Get current BGM volume
     */
    float getBGMVolume() const { return bgmVolume; }

    // ========== SE Management ==========

    /**
     * @brief Play SE (sound effect)
     * @param clipId Loaded clip ID
     * @param position 3D playback position (default: listener position)
     * @param volume Volume (default: 1.0)
     * @return Source ID (0 on failure)
     */
    uint32_t playSE(uint32_t clipId, const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
                   float volume = 1.0f);

    /**
     * @brief Stop SE
     * @param sourceId Source ID
     */
    void stopSE(uint32_t sourceId);

    /**
     * @brief Stop all SE
     */
    void stopAllSE();

    // ========== 3D Audio ==========

    /**
     * @brief Set listener (player) position
     * @param pos World coordinates
     */
    void setListenerPosition(const glm::vec3& pos);

    /**
     * @brief Set listener orientation
     * @param forward Forward vector
     * @param up Up vector
     */
    void setListenerOrientation(const glm::vec3& forward, const glm::vec3& up);

    // ========== Volume Control ==========

    /**
     * @brief Set master volume (overall)
     * @param volume 0.0 - 1.0
     */
    void setMasterVolume(float volume);

    /**
     * @brief Get master volume
     */
    float getMasterVolume() const { return masterVolume; }

    /**
     * @brief Set SE master volume
     * @param volume 0.0 - 1.0
     */
    void setSEVolume(float volume);

    /**
     * @brief Get SE master volume
     */
    float getSEVolume() const { return seVolume; }

    // ========== Status Query ==========

    /**
     * @brief Number of currently loaded clips
     */
    size_t getLoadedClipsCount() const { return clips.size(); }

    /**
     * @brief Number of currently playing sources
     */
    size_t getActiveSourcesesCount() const { return sources.size(); }

    /**
     * @brief Get list of loaded audio
     */
    std::string getLoadedAudioList() const;

    /**
     * @brief Get audio statistics
     */
    std::string getAudioStats() const;

    // ========== Sound Definition JSON ==========

    /**
     * @brief Load sound_definitions.json and register sound definitions
     * @param jsonPath JSON file path (e.g. "audio/sound_definitions.json")
     * @return True on success
     */
    bool loadSoundDefinitions(const std::string& jsonPath);

    /**
     * @brief Play SE by definition key (registered in JSON)
     * @param key Sound key defined in JSON (e.g. "ui/click")
     * @param position 3D position (listener position if omitted)
     * @return Source ID (0 on failure)
     */
    uint32_t playSound(const std::string& key,
                       const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f));

    /**
     * @brief Play BGM by definition key
     * @param key BGM key defined in JSON
     * @param fadeIn Fade in time
     * @return True on success
     */
    bool playMusic(const std::string& key, float fadeIn = 0.0f);

    /**
     * @brief Check if sound definitions are loaded
     */
    bool hasSoundDefinitions() const { return !soundDefs.empty(); }

    /**
     * @brief Sound definition structure
     */
    struct SoundDef {
        std::string file;
        uint8_t type;      // 0=BGM, 1=SE
        float volume;
        bool loop;
        bool is3D;
        uint32_t clipId;   // Set after loading
    };

    /**
     * @brief Get sound definitions map (read-only)
     */
    const std::unordered_map<std::string, SoundDef>& getSoundDefs() const { return soundDefs; }

private:
    // ... existing private members ...

    std::unordered_map<std::string, SoundDef> soundDefs;

    bool parseSoundDefinitionsJson(const std::string& jsonContent);
    std::string extractJsonString(const std::string& json, size_t& pos);
    std::string extractJsonValue(const std::string& json, const std::string& key, size_t startPos);

public:
    static constexpr uint32_t MAX_SOURCES = 32;
    static constexpr uint32_t JAVA_BGM_SOURCE_ID = 0xFFFFFFFE;  // Sentinel for Java MediaPlayer BGM
    static constexpr float FADE_UPDATE_RATE = 0.016f;  // 60 FPS

    void playBGMViaJava(const std::string& filename);

    // OpenAL device & context
    ALCdevice* device;
    ALCcontext* context;

    // 3D Audio system
    std::unique_ptr<Audio3D> audio3D;

    // Resource management
    std::unordered_map<uint32_t, std::shared_ptr<AudioClip>> clips;  // clipId -> Clip
    std::unordered_map<uint32_t, std::shared_ptr<AudioSource>> sources;  // sourceId -> Source

    uint32_t nextClipId;
    uint32_t nextSourceId;

    // BGM state
    uint32_t currentBGMSourceId;
    uint32_t currentBGMClipId;
    float bgmVolume;
    float bgmFadeTarget;
    float bgmFadeRate;
    bool bgmFading;

    // Volume levels
    float masterVolume;
    float seVolume;

    // ========== Private Methods ==========

    /**
     * @brief Load WAV file
     * @param filename File path
     * @return OpenAL buffer handle (0 = failure)
     */
    ALuint loadWavFile(const std::string& filename, ALint& format,
                      ALsizei& frequency, ALsizei& size);

    /**
     * @brief Clean up completed playback sources
     */
    void cleanupFinishedSources();

    /**
     * @brief BGM fade processing
     * @param deltaTime Frame time
     */
    void updateBGMFade(float deltaTime);

    /**
     * @brief Create new source
     * @param clipId Clip ID
     * @return Created source ID
     */
    uint32_t createSource(uint32_t clipId);

    /**
     * @brief Delete source
     * @param sourceId Source ID
     */
    void destroySource(uint32_t sourceId);
};
