#pragma once

// Sound Effect Manager
// Manages SFX playback with pooling, priority-based voice limiting,
// distance-based attenuation, Doppler stub, and environmental reverb presets.

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <glm/glm.hpp>
#include <android/log.h>

#include "audio_config.h"

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "SfxManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declaration
class AudioManager;

namespace audio {

/**
 * @brief Sound effect priority levels
 * Higher priority sounds are less likely to be evicted.
 */
enum class SfxPriority : uint8_t {
    LOW = 0,        // Ambient, distant sounds
    NORMAL = 1,     // Standard gameplay SFX
    HIGH = 2,       // Important combat/UI sounds
    CRITICAL = 3    // Player hit, death, quest-critical
};

/**
 * @brief Sound effect descriptor
 */
struct SfxDef {
    std::string key;            // Unique identifier
    std::string filePath;       // Asset path
    float baseVolume;           // Default volume (0.0 - 1.0)
    float minDistance;           // Distance at which volume is 1.0
    float maxDistance;           // Distance beyond which sound is inaudible
    SfxPriority priority;       // Priority for voice limiting
    bool is3D;                  // Whether to use 3D positioning
    bool preload;               // Whether to preload into memory
    uint32_t clipId;            // Loaded clip ID (set after loading)

    SfxDef()
        : baseVolume(1.0f), minDistance(1.0f), maxDistance(50.0f),
          priority(SfxPriority::NORMAL), is3D(true), preload(false), clipId(0) {
    }
};

/**
 * @brief Active sound effect instance
 */
struct SfxInstance {
    uint32_t instanceId;
    uint32_t sourceId;          // AudioManager source ID
    std::string key;            // SfxDef key
    glm::vec3 position;         // World position
    float volume;               // Effective volume
    SfxPriority priority;
    float age;                  // Time since playback started
    bool active;

    SfxInstance()
        : instanceId(0), sourceId(0),
          position(0.0f, 0.0f, 0.0f), volume(1.0f),
          priority(SfxPriority::NORMAL), age(0.0f), active(false) {
    }
};

/**
 * @brief Doppler effect configuration (stub)
 */
struct DopplerConfig {
    float factor;               // 0.0 = disabled, 1.0 = natural
    float speedOfSound;         // m/s (default 343.0)

    DopplerConfig() : factor(0.0f), speedOfSound(343.0f) {
    }
};

/**
 * @brief Sound Effect Manager
 * Manages SFX pools, priority-based voice limiting, distance attenuation,
 * and environmental reverb presets.
 */
class SoundEffectManager {
public:
    /**
     * @brief Maximum simultaneous SFX voices
     */
    static constexpr uint32_t MAX_VOICES = 24;

    SoundEffectManager();
    ~SoundEffectManager();

    /**
     * @brief Initialize with AudioManager reference
     * @param manager Pointer to the main AudioManager
     */
    void initialize(AudioManager* manager);

    /**
     * @brief Update active SFX instances (call every frame)
     * @param deltaTime Frame time in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    // ========== SFX Registration ==========

    /**
     * @brief Register a sound effect definition
     * @param def SFX descriptor
     */
    void registerSfx(const SfxDef& def);

    /**
     * @brief Register a simple SFX with key and path
     * @param key Unique key
     * @param filePath Asset path
     * @param volume Base volume
     * @param priority Playback priority
     */
    void registerSfx(const std::string& key, const std::string& filePath,
                     float volume = 1.0f,
                     SfxPriority priority = SfxPriority::NORMAL);

    /**
     * @brief Preload all SFX marked with preload=true
     */
    void preloadAll();

    /**
     * @brief Check if an SFX is registered
     */
    bool hasSfx(const std::string& key) const;

    // ========== Playback ==========

    /**
     * @brief Play a sound effect at a 3D position
     * @param key SFX key
     * @param position World position
     * @param volumeScale Volume multiplier (0.0 - 1.0)
     * @return Instance ID (0 = failed)
     */
    uint32_t play(const std::string& key,
                  const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
                  float volumeScale = 1.0f);

    /**
     * @brief Play a sound effect as 2D (non-positional)
     * @param key SFX key
     * @param volumeScale Volume multiplier
     * @return Instance ID (0 = failed)
     */
    uint32_t play2D(const std::string& key, float volumeScale = 1.0f);

    /**
     * @brief Stop a specific SFX instance
     * @param instanceId Instance ID from play()
     */
    void stop(uint32_t instanceId);

    /**
     * @brief Stop all active SFX
     */
    void stopAll();

    // ========== Volume ==========

    /**
     * @brief Set SFX master volume
     * @param volume 0.0 - 1.0
     */
    void setVolume(float volume);

    /**
     * @brief Get SFX master volume
     */
    float getVolume() const { return sfxVolume; }

    // ========== Distance Model ==========

    /**
     * @brief Set distance attenuation parameters
     * @param refDist Reference distance
     * @param maxDist Maximum distance
     * @param rolloff Rolloff factor
     */
    void setDistanceModel(float refDist, float maxDist, float rolloff);

    /**
     * @brief Calculate volume attenuation based on distance
     * @param sourcePos Source position
     * @param listenerPos Listener position
     * @param refDist Reference distance
     * @param maxDist Maximum distance
     * @param rolloff Rolloff factor
     * @return Attenuation factor (0.0 - 1.0)
     */
    static float calculateAttenuation(const glm::vec3& sourcePos,
                                       const glm::vec3& listenerPos,
                                       float refDist, float maxDist, float rolloff);

    // ========== Doppler Effect (Stub) ==========

    /**
     * @brief Configure Doppler effect
     * @param config Doppler configuration
     */
    void setDopplerConfig(const DopplerConfig& config);

    /**
     * @brief Calculate Doppler pitch shift (stub)
     * @param sourcePos Source position
     * @param sourceVel Source velocity
     * @param listenerPos Listener position
     * @param listenerVel Listener velocity
     * @return Pitch multiplier (1.0 = no shift)
     */
    float calculateDoppler(const glm::vec3& sourcePos, const glm::vec3& sourceVel,
                           const glm::vec3& listenerPos, const glm::vec3& listenerVel) const;

    // ========== Environmental Reverb ==========

    /**
     * @brief Set current environmental reverb preset
     * @param preset Reverb preset type
     */
    void setReverbPreset(ReverbPreset preset);

    /**
     * @brief Get current reverb configuration
     */
    const ReverbConfig& getReverbConfig() const { return currentReverb; }

    /**
     * @brief Set listener position for distance calculations
     */
    void setListenerPosition(const glm::vec3& pos) { listenerPosition = pos; }

    /**
     * @brief Get active voice count
     */
    uint32_t getActiveVoiceCount() const;

private:
    AudioManager* audioManager;

    // SFX definitions
    std::unordered_map<std::string, SfxDef> sfxDefs;

    // Active instances
    std::vector<SfxInstance> activeInstances;
    uint32_t nextInstanceId;

    // Settings
    float sfxVolume;
    float refDistance;
    float maxDistance;
    float rolloffFactor;

    // Listener
    glm::vec3 listenerPosition;

    // Doppler
    DopplerConfig dopplerConfig;

    // Reverb
    ReverbConfig currentReverb;

    // Internal methods
    uint32_t findFreeVoice();
    uint32_t evictLowestPriorityVoice(SfxPriority requiredPriority);
    void updateInstanceVolumes();
    float computeInstanceVolume(const SfxInstance& inst, const SfxDef& def) const;
};

} // namespace audio
