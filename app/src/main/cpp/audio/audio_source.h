#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#ifdef AUDIO_SYSTEM_ENABLED
#include <AL/al.h>
#else
// Stub: OpenAL not needed for Java MediaPlayer approach via JNI
using ALuint = unsigned int;
#endif

/**
 * @brief Audio playback channel (AudioSource)
 * Manages OpenAL source and controls individual audio playback
 *
 * Represents individual playback channels for BGM, SE, voice, etc.
 * Manages 3D position, volume, pitch, and playback state
 */
struct AudioSource {
    uint32_t sourceId;              // Unique ID (assigned by Renderer)
    ALuint alSource;                // OpenAL source handle
    uint32_t clipId;                // Currently playing clip ID

    // 3D Audio
    glm::vec3 position;             // World coordinates
    glm::vec3 velocity;             // For Doppler effect

    // Volume & Pitch
    float volume;                   // 0.0 - 1.0
    float pitch;                    // 0.5 - 2.0

    // Playback state
    bool isPlaying;
    float playbackTime;             // Playback elapsed time (seconds)

    // Flags
    bool isLooping;                 // Loop playback flag
    bool is3D;                      // 3D audio flag

    /**
     * @brief Default constructor
     */
    AudioSource()
        : sourceId(0), alSource(0), clipId(0),
          position(0.0f, 0.0f, 0.0f), velocity(0.0f, 0.0f, 0.0f),
          volume(1.0f), pitch(1.0f),
          isPlaying(false), playbackTime(0.0f),
          isLooping(false), is3D(false) {
    }

    /**
     * @brief Set volume
     * @param vol 0.0 - 1.0
     */
    void setVolume(float vol) {
        volume = vol < 0.0f ? 0.0f : (vol > 1.0f ? 1.0f : vol);
        #ifdef AUDIO_SYSTEM_ENABLED
        if (alSource != 0) {
            alSourcef(alSource, AL_GAIN, volume);
        }
        #endif
    }

    /**
     * @brief Set pitch
     * @param p 0.5 - 2.0
     */
    void setPitch(float p) {
        pitch = p < 0.5f ? 0.5f : (p > 2.0f ? 2.0f : p);
        #ifdef AUDIO_SYSTEM_ENABLED
        if (alSource != 0) {
            alSourcef(alSource, AL_PITCH, pitch);
        }
        #endif
    }

    /**
     * @brief Set 3D position
     * @param pos World coordinates
     */
    void setPosition(const glm::vec3& pos) {
        position = pos;
        #ifdef AUDIO_SYSTEM_ENABLED
        if (alSource != 0 && is3D) {
            alSource3f(alSource, AL_POSITION, pos.x, pos.y, pos.z);
        }
        #endif
    }

    /**
     * @brief Set velocity (for Doppler effect)
     * @param vel Velocity vector
     */
    void setVelocity(const glm::vec3& vel) {
        velocity = vel;
        #ifdef AUDIO_SYSTEM_ENABLED
        if (alSource != 0 && is3D) {
            alSource3f(alSource, AL_VELOCITY, vel.x, vel.y, vel.z);
        }
        #endif
    }

    /**
     * @brief Enable 3D audio
     */
    void enable3D() {
        is3D = true;
        #ifdef AUDIO_SYSTEM_ENABLED
        if (alSource != 0) {
            alSourcei(alSource, AL_SOURCE_RELATIVE, AL_FALSE);
        }
        #endif
    }

    /**
     * @brief Disable 3D audio (monaural playback)
     */
    void disable3D() {
        is3D = false;
        #ifdef AUDIO_SYSTEM_ENABLED
        if (alSource != 0) {
            alSourcei(alSource, AL_SOURCE_RELATIVE, AL_TRUE);
            position = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        #endif
    }
};
