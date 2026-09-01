#pragma once

#include <string>
#include <cstdint>

// Stub: OpenAL not needed for Java MediaPlayer approach via JNI
using ALuint = unsigned int;

/**
 * @brief Audio resource (clip)
 * Represents audio data such as WAV/OGG
 */
struct AudioClip {
    uint32_t clipId;                // Unique ID
    std::string filename;            // File path
    ALuint alBuffer;                 // OpenAL buffer handle
    float duration;                  // Playback duration (seconds)
    bool isLooping;                  // Loop playback flag
    bool isStreamed;                 // Streaming playback (true) vs full load (false)
    float volume;                    // Base volume (0.0 - 1.0)
    uint8_t type;                    // Audio type: 0=BGM, 1=SE, 2=Voice

    /**
     * @brief Constructor
     */
    AudioClip()
        : clipId(0), alBuffer(0), duration(0.0f), isLooping(false),
          isStreamed(false), volume(1.0f), type(1) {
    }

    /**
     * @brief Destructor
     */
    ~AudioClip() {
        // JNI bridge handles resource cleanup via Java MediaPlayer
        alBuffer = 0;
    }
};
