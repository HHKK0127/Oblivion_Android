#pragma once

// Audio Configuration
// Centralized audio settings for master volume, per-channel volumes,
// distance model, and listener orientation.

#include <cstdint>
#include <glm/glm.hpp>

namespace audio {

/**
 * @brief Master audio settings struct
 * Holds all volume levels and audio configuration parameters.
 * Serializable for save/load support.
 */
struct AudioSettings {
    // Volume levels (0.0 - 1.0)
    float masterVolume;
    float bgmVolume;
    float sfxVolume;
    float voiceVolume;
    float ambientVolume;

    // Distance model parameters
    float referenceDistance;    // Distance at which volume is 1.0
    float maxDistance;          // Distance beyond which volume is 0.0
    float rolloffFactor;       // How quickly volume decreases with distance

    // Doppler settings
    float dopplerFactor;       // 0.0 = disabled, 1.0 = natural
    float speedOfSound;        // m/s (default 343.0)

    // Listener orientation
    glm::vec3 listenerForward;
    glm::vec3 listenerUp;

    // Feature flags
    bool enable3DAudio;
    bool enableReverb;
    bool enableDoppler;

    /**
     * @brief Default constructor with sensible defaults
     */
    AudioSettings()
        : masterVolume(1.0f),
          bgmVolume(0.7f),
          sfxVolume(1.0f),
          voiceVolume(1.0f),
          ambientVolume(0.5f),
          referenceDistance(1.0f),
          maxDistance(100.0f),
          rolloffFactor(1.0f),
          dopplerFactor(1.0f),
          speedOfSound(343.0f),
          listenerForward(0.0f, 0.0f, -1.0f),
          listenerUp(0.0f, 1.0f, 0.0f),
          enable3DAudio(true),
          enableReverb(true),
          enableDoppler(false) {
    }

    /**
     * @brief Clamp all volume values to valid range
     */
    void clamp() {
        auto clampf = [](float v, float lo, float hi) -> float {
            return v < lo ? lo : (v > hi ? hi : v);
        };
        masterVolume = clampf(masterVolume, 0.0f, 1.0f);
        bgmVolume = clampf(bgmVolume, 0.0f, 1.0f);
        sfxVolume = clampf(sfxVolume, 0.0f, 1.0f);
        voiceVolume = clampf(voiceVolume, 0.0f, 1.0f);
        ambientVolume = clampf(ambientVolume, 0.0f, 1.0f);
        referenceDistance = clampf(referenceDistance, 0.1f, 100.0f);
        maxDistance = clampf(maxDistance, 1.0f, 10000.0f);
        rolloffFactor = clampf(rolloffFactor, 0.0f, 10.0f);
        dopplerFactor = clampf(dopplerFactor, 0.0f, 2.0f);
        speedOfSound = clampf(speedOfSound, 1.0f, 10000.0f);
    }
};

/**
 * @brief Environmental reverb presets for different game areas
 */
enum class ReverbPreset : uint8_t {
    NONE = 0,
    CAVE,
    DUNGEON,
    OUTDOOR,
    INDOOR,
    HALL,
    FOREST,
    MOUNTAIN
};

/**
 * @brief Reverb parameters for environmental audio
 */
struct ReverbConfig {
    ReverbPreset preset;
    float roomSize;         // 0.0 - 1.0
    float damping;          // 0.0 - 1.0
    float wetLevel;         // 0.0 - 1.0 (reverb mix)
    float dryLevel;         // 0.0 - 1.0 (direct sound mix)
    float decayTime;        // seconds

    ReverbConfig()
        : preset(ReverbPreset::NONE),
          roomSize(0.5f), damping(0.5f),
          wetLevel(0.3f), dryLevel(0.7f),
          decayTime(1.0f) {
    }

    /**
     * @brief Get preset configuration for a given environment type
     */
    static ReverbConfig getPreset(ReverbPreset p) {
        ReverbConfig cfg;
        cfg.preset = p;
        switch (p) {
            case ReverbPreset::CAVE:
                cfg.roomSize = 0.8f;
                cfg.damping = 0.3f;
                cfg.wetLevel = 0.6f;
                cfg.dryLevel = 0.5f;
                cfg.decayTime = 2.5f;
                break;
            case ReverbPreset::DUNGEON:
                cfg.roomSize = 0.6f;
                cfg.damping = 0.4f;
                cfg.wetLevel = 0.5f;
                cfg.dryLevel = 0.6f;
                cfg.decayTime = 1.8f;
                break;
            case ReverbPreset::OUTDOOR:
                cfg.roomSize = 1.0f;
                cfg.damping = 0.8f;
                cfg.wetLevel = 0.1f;
                cfg.dryLevel = 0.9f;
                cfg.decayTime = 0.3f;
                break;
            case ReverbPreset::INDOOR:
                cfg.roomSize = 0.4f;
                cfg.damping = 0.6f;
                cfg.wetLevel = 0.3f;
                cfg.dryLevel = 0.7f;
                cfg.decayTime = 0.8f;
                break;
            case ReverbPreset::HALL:
                cfg.roomSize = 0.9f;
                cfg.damping = 0.3f;
                cfg.wetLevel = 0.5f;
                cfg.dryLevel = 0.6f;
                cfg.decayTime = 2.0f;
                break;
            case ReverbPreset::FOREST:
                cfg.roomSize = 0.7f;
                cfg.damping = 0.7f;
                cfg.wetLevel = 0.2f;
                cfg.dryLevel = 0.8f;
                cfg.decayTime = 0.5f;
                break;
            case ReverbPreset::MOUNTAIN:
                cfg.roomSize = 1.0f;
                cfg.damping = 0.2f;
                cfg.wetLevel = 0.4f;
                cfg.dryLevel = 0.6f;
                cfg.decayTime = 3.0f;
                break;
            default:
                break;
        }
        return cfg;
    }
};

} // namespace audio
