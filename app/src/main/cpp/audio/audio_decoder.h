#pragma once

// Audio Decoder
// Decodes WAV (PCM 8/16/24-bit, mono/stereo) audio files.
// Provides stubs for MP3 and OGG Vorbis format detection.
// Includes a ring buffer for streaming and basic sample rate conversion.

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "AudioDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace audio {

/**
 * @brief Supported audio formats
 */
enum class AudioFormat : uint8_t {
    UNKNOWN = 0,
    WAV_PCM,
    MP3,
    OGG_VORBIS
};

/**
 * @brief Decoded audio data descriptor
 */
struct AudioData {
    std::vector<uint8_t> pcmData;      // Raw PCM samples
    uint32_t sampleRate;                // Hz (e.g. 44100)
    uint16_t bitsPerSample;             // 8, 16, or 24
    uint16_t numChannels;               // 1 = mono, 2 = stereo
    uint32_t totalSamples;              // Number of sample frames
    float duration;                     // Duration in seconds
    AudioFormat format;                 // Detected format

    AudioData()
        : sampleRate(0), bitsPerSample(0), numChannels(0),
          totalSamples(0), duration(0.0f), format(AudioFormat::UNKNOWN) {
    }

    /**
     * @brief Get bytes per sample frame (all channels)
     */
    uint32_t bytesPerFrame() const {
        return (bitsPerSample / 8) * numChannels;
    }

    /**
     * @brief Check if data is valid
     */
    bool isValid() const {
        return !pcmData.empty() && sampleRate > 0 && numChannels > 0;
    }
};

/**
 * @brief Ring buffer for audio streaming
 * Provides lock-free single-producer single-consumer buffering.
 */
class AudioRingBuffer {
public:
    /**
     * @brief Construct ring buffer with given capacity in bytes
     * @param capacity Buffer size in bytes
     */
    explicit AudioRingBuffer(size_t capacity = 65536);

    ~AudioRingBuffer();

    /**
     * @brief Write data into the ring buffer
     * @param data Source data pointer
     * @param size Number of bytes to write
     * @return Number of bytes actually written
     */
    size_t write(const uint8_t* data, size_t size);

    /**
     * @brief Read data from the ring buffer
     * @param data Destination buffer
     * @param size Maximum bytes to read
     * @return Number of bytes actually read
     */
    size_t read(uint8_t* data, size_t size);

    /**
     * @brief Get number of bytes available to read
     */
    size_t availableRead() const;

    /**
     * @brief Get number of bytes available to write
     */
    size_t availableWrite() const;

    /**
     * @brief Reset the buffer to empty state
     */
    void reset();

    /**
     * @brief Check if buffer is empty
     */
    bool isEmpty() const { return readPos == writePos; }

    /**
     * @brief Get total capacity
     */
    size_t capacity() const { return bufferSize; }

private:
    std::vector<uint8_t> buffer;
    size_t bufferSize;
    size_t readPos;
    size_t writePos;
};

/**
 * @brief Audio decoder class
 * Decodes audio files from raw byte data or AAsset paths.
 */
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    /**
     * @brief Detect audio format from file header bytes
     * @param header First 12+ bytes of the file
     * @param size Number of bytes available
     * @return Detected format
     */
    static AudioFormat detectFormat(const uint8_t* header, size_t size);

    /**
     * @brief Decode WAV PCM audio from raw file data
     * @param fileData Complete file contents
     * @param fileSize Size in bytes
     * @return Decoded audio data (empty on failure)
     */
    AudioData decodeWav(const uint8_t* fileData, size_t fileSize);

    /**
     * @brief Parse MP3 header for format info (stub)
     * @param fileData File contents
     * @param fileSize Size in bytes
     * @return AudioData with format info only (no PCM decode)
     */
    AudioData parseMp3Header(const uint8_t* fileData, size_t fileSize);

    /**
     * @brief Parse OGG Vorbis header for format info (stub)
     * @param fileData File contents
     * @param fileSize Size in bytes
     * @return AudioData with format info only (no PCM decode)
     */
    AudioData parseOggHeader(const uint8_t* fileData, size_t fileSize);

    /**
     * @brief Decode audio from raw file data (auto-detect format)
     * @param fileData Complete file contents
     * @param fileSize Size in bytes
     * @return Decoded audio data
     */
    AudioData decode(const uint8_t* fileData, size_t fileSize);

    /**
     * @brief Convert sample rate using linear interpolation
     * @param input Input PCM data
     * @param inputRate Source sample rate
     * @param outputRate Target sample rate
     * @return Resampled PCM data (16-bit)
     */
    static std::vector<int16_t> resample(const std::vector<int16_t>& input,
                                          uint32_t inputRate,
                                          uint32_t outputRate);

    /**
     * @brief Convert 24-bit PCM to 16-bit
     * @param src Source 24-bit samples
     * @param sampleCount Number of samples
     * @return 16-bit samples
     */
    static std::vector<int16_t> convert24to16(const uint8_t* src, size_t sampleCount);

    /**
     * @brief Convert 8-bit unsigned PCM to 16-bit signed
     * @param src Source 8-bit samples
     * @param sampleCount Number of samples
     * @return 16-bit samples
     */
    static std::vector<int16_t> convert8to16(const uint8_t* src, size_t sampleCount);

private:
    // Little-endian read helpers
    static uint16_t readU16LE(const uint8_t* data, size_t offset);
    static uint32_t readU32LE(const uint8_t* data, size_t offset);
    static int16_t readI16LE(const uint8_t* data, size_t offset);
};

} // namespace audio
