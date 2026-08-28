#include "audio_decoder.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace audio {

// ============================================================================
// AudioRingBuffer
// ============================================================================

AudioRingBuffer::AudioRingBuffer(size_t capacity)
    : buffer(capacity, 0), bufferSize(capacity), readPos(0), writePos(0) {
}

AudioRingBuffer::~AudioRingBuffer() {
}

size_t AudioRingBuffer::write(const uint8_t* data, size_t size) {
    size_t avail = availableWrite();
    size_t toWrite = std::min(size, avail);

    for (size_t i = 0; i < toWrite; ++i) {
        buffer[writePos] = data[i];
        writePos = (writePos + 1) % bufferSize;
    }

    return toWrite;
}

size_t AudioRingBuffer::read(uint8_t* data, size_t size) {
    size_t avail = availableRead();
    size_t toRead = std::min(size, avail);

    for (size_t i = 0; i < toRead; ++i) {
        data[i] = buffer[readPos];
        readPos = (readPos + 1) % bufferSize;
    }

    return toRead;
}

size_t AudioRingBuffer::availableRead() const {
    if (writePos >= readPos) {
        return writePos - readPos;
    }
    return bufferSize - readPos + writePos;
}

size_t AudioRingBuffer::availableWrite() const {
    return bufferSize - 1 - availableRead();
}

void AudioRingBuffer::reset() {
    readPos = 0;
    writePos = 0;
}

// ============================================================================
// AudioDecoder - Little-endian read helpers
// ============================================================================

uint16_t AudioDecoder::readU16LE(const uint8_t* data, size_t offset) {
    return static_cast<uint16_t>(data[offset]) |
           (static_cast<uint16_t>(data[offset + 1]) << 8);
}

uint32_t AudioDecoder::readU32LE(const uint8_t* data, size_t offset) {
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}

int16_t AudioDecoder::readI16LE(const uint8_t* data, size_t offset) {
    return static_cast<int16_t>(readU16LE(data, offset));
}

// ============================================================================
// AudioDecoder - Format detection
// ============================================================================

AudioFormat AudioDecoder::detectFormat(const uint8_t* header, size_t size) {
    if (size < 12) {
        return AudioFormat::UNKNOWN;
    }

    // RIFF....WAVE = WAV
    if (header[0] == 'R' && header[1] == 'I' &&
        header[2] == 'F' && header[3] == 'F' &&
        header[8] == 'W' && header[9] == 'A' &&
        header[10] == 'V' && header[11] == 'E') {
        return AudioFormat::WAV_PCM;
    }

    // MP3 sync word: 0xFF 0xFB/0xFA/0xF3/0xF2
    if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) {
        return AudioFormat::MP3;
    }

    // ID3 tag (MP3 with ID3 header)
    if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
        return AudioFormat::MP3;
    }

    // OGG Vorbis
    if (header[0] == 'O' && header[1] == 'g' &&
        header[2] == 'g' && header[3] == 'S') {
        return AudioFormat::OGG_VORBIS;
    }

    return AudioFormat::UNKNOWN;
}

// ============================================================================
// AudioDecoder - WAV decoder
// ============================================================================

AudioData AudioDecoder::decodeWav(const uint8_t* fileData, size_t fileSize) {
    AudioData result;

    if (fileSize < 44) {
        LOGE("WAV file too small: %lu bytes", static_cast<unsigned long>(fileSize));
        return result;
    }

    // RIFF header
    if (readU32LE(fileData, 0) != 0x46464952) {  // "RIFF"
        LOGE("Invalid WAV: no RIFF header");
        return result;
    }

    if (readU32LE(fileData, 8) != 0x45564157) {  // "WAVE"
        LOGE("Invalid WAV: no WAVE marker");
        return result;
    }

    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t audioDataOffset = 0;
    uint32_t audioDataSize = 0;

    // Parse chunks
    size_t pos = 12;
    while (pos + 8 <= fileSize) {
        uint32_t chunkId = readU32LE(fileData, pos);
        uint32_t chunkSize = readU32LE(fileData, pos + 4);

        if (pos + 8 + chunkSize > fileSize) {
            LOGE("Invalid chunk size %u at offset %lu", chunkSize,
                 static_cast<unsigned long>(pos));
            return result;
        }

        if (chunkId == 0x20746D66) {  // "fmt "
            if (chunkSize < 16) {
                LOGE("Invalid fmt chunk size: %u", chunkSize);
                return result;
            }
            uint16_t audioFormat = readU16LE(fileData, pos + 8);
            if (audioFormat != 1) {  // PCM only
                LOGE("Unsupported WAV format: %u (only PCM=1)", audioFormat);
                return result;
            }
            numChannels = readU16LE(fileData, pos + 10);
            sampleRate = readU32LE(fileData, pos + 12);
            bitsPerSample = readU16LE(fileData, pos + 22);

            LOGD("WAV fmt: ch=%u sr=%u bits=%u", numChannels, sampleRate, bitsPerSample);
        } else if (chunkId == 0x61746164) {  // "data"
            audioDataOffset = static_cast<uint32_t>(pos + 8);
            audioDataSize = chunkSize;
        }

        // Next chunk with 2-byte alignment padding
        pos += 8 + chunkSize + (chunkSize % 2);
    }

    if (audioDataSize == 0 || numChannels == 0 || sampleRate == 0) {
        LOGE("Invalid WAV: missing fmt or data chunk");
        return result;
    }

    if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24) {
        LOGE("Unsupported bits per sample: %u", bitsPerSample);
        return result;
    }

    if (numChannels > 2) {
        LOGE("Unsupported channel count: %u (max 2)", numChannels);
        return result;
    }

    // Normalize to 16-bit PCM
    size_t sampleCount = audioDataSize / (bitsPerSample / 8);
    std::vector<int16_t> pcm16;

    if (bitsPerSample == 16) {
        pcm16.resize(sampleCount);
        memcpy(pcm16.data(), fileData + audioDataOffset, sampleCount * sizeof(int16_t));
    } else if (bitsPerSample == 8) {
        pcm16 = convert8to16(fileData + audioDataOffset, sampleCount);
    } else if (bitsPerSample == 24) {
        pcm16 = convert24to16(fileData + audioDataOffset, sampleCount);
    }

    // Store as 16-bit PCM
    result.pcmData.resize(pcm16.size() * sizeof(int16_t));
    memcpy(result.pcmData.data(), pcm16.data(), result.pcmData.size());
    result.sampleRate = sampleRate;
    result.bitsPerSample = 16;  // Always output 16-bit
    result.numChannels = numChannels;
    result.totalSamples = static_cast<uint32_t>(pcm16.size() / numChannels);
    result.duration = static_cast<float>(result.totalSamples) / static_cast<float>(sampleRate);
    result.format = AudioFormat::WAV_PCM;

    LOGI("WAV decoded: %uch %uHz %ubits, %.2fs, %lu bytes PCM",
         numChannels, sampleRate, bitsPerSample, result.duration,
         static_cast<unsigned long>(result.pcmData.size()));

    return result;
}

// ============================================================================
// AudioDecoder - MP3 header parser (stub)
// ============================================================================

AudioData AudioDecoder::parseMp3Header(const uint8_t* fileData, size_t fileSize) {
    AudioData result;
    result.format = AudioFormat::MP3;

    if (fileSize < 4) {
        LOGE("MP3 file too small");
        return result;
    }

    // Skip ID3v2 tag if present
    size_t dataOffset = 0;
    if (fileData[0] == 'I' && fileData[1] == 'D' && fileData[2] == '3') {
        if (fileSize >= 10) {
            uint32_t tagSize = (static_cast<uint32_t>(fileData[6]) << 21) |
                               (static_cast<uint32_t>(fileData[7]) << 14) |
                               (static_cast<uint32_t>(fileData[8]) << 7) |
                               static_cast<uint32_t>(fileData[9]);
            dataOffset = 10 + tagSize;
            LOGD("ID3v2 tag found, size=%u, data starts at %lu",
                 tagSize, static_cast<unsigned long>(dataOffset));
        }
    }

    // Find first sync frame
    size_t syncPos = dataOffset;
    while (syncPos + 4 <= fileSize) {
        if (fileData[syncPos] == 0xFF && (fileData[syncPos + 1] & 0xE0) == 0xE0) {
            break;
        }
        syncPos++;
    }

    if (syncPos + 4 > fileSize) {
        LOGE("MP3: no sync frame found");
        return result;
    }

    // Parse MPEG audio header
    uint8_t versionBits = (fileData[syncPos + 1] >> 3) & 0x03;
    uint8_t layerBits = (fileData[syncPos + 1] >> 1) & 0x03;
    uint8_t bitrateIndex = (fileData[syncPos + 2] >> 4) & 0x0F;
    uint8_t sampleRateIndex = (fileData[syncPos + 2] >> 2) & 0x03;
    uint8_t channelMode = (fileData[syncPos + 3] >> 6) & 0x03;

    // Sample rate table (MPEG1/2/2.5)
    static const uint32_t sampleRateTable[3][4] = {
        {44100, 48000, 32000, 0},  // MPEG1
        {22050, 24000, 16000, 0},  // MPEG2
        {11025, 12000, 8000, 0}    // MPEG2.5
    };

    int versionIndex = -1;
    if (versionBits == 3) versionIndex = 0;       // MPEG1
    else if (versionBits == 2) versionIndex = 1;  // MPEG2
    else if (versionBits == 0) versionIndex = 2;  // MPEG2.5

    if (versionIndex >= 0 && sampleRateIndex < 3) {
        result.sampleRate = sampleRateTable[versionIndex][sampleRateIndex];
    }

    result.numChannels = (channelMode == 3) ? 1 : 2;
    result.bitsPerSample = 16;  // MP3 always decodes to 16-bit
    result.format = AudioFormat::MP3;

    LOGI("MP3 header parsed: version=%u layer=%u sr=%u ch=%u (decode not implemented)",
         versionBits, layerBits, result.sampleRate, result.numChannels);

    return result;
}

// ============================================================================
// AudioDecoder - OGG Vorbis header parser (stub)
// ============================================================================

AudioData AudioDecoder::parseOggHeader(const uint8_t* fileData, size_t fileSize) {
    AudioData result;
    result.format = AudioFormat::OGG_VORBIS;

    if (fileSize < 36) {
        LOGE("OGG file too small");
        return result;
    }

    // OGG page header: "OggS" at offset 0
    if (fileData[0] != 'O' || fileData[1] != 'g' ||
        fileData[2] != 'g' || fileData[3] != 'S') {
        LOGE("Invalid OGG header");
        return result;
    }

    // Vorbis identification header is in the second segment of first page
    // Look for "vorbis" marker at offset 29 (after OGG page header + vorbis header)
    size_t vorbisOffset = 0;
    for (size_t i = 28; i + 6 < fileSize; ++i) {
        if (fileData[i] == 'v' && fileData[i + 1] == 'o' &&
            fileData[i + 2] == 'r' && fileData[i + 3] == 'b' &&
            fileData[i + 4] == 'i' && fileData[i + 5] == 's') {
            vorbisOffset = i;
            break;
        }
    }

    if (vorbisOffset > 0 && vorbisOffset + 23 < fileSize) {
        // Vorbis identification header: version(4) + channels(1) + sample_rate(4)
        result.numChannels = fileData[vorbisOffset + 11];
        result.sampleRate = readU32LE(fileData, vorbisOffset + 12);
        result.bitsPerSample = 16;  // Vorbis decodes to 16-bit
        result.format = AudioFormat::OGG_VORBIS;

        LOGI("OGG Vorbis header parsed: sr=%u ch=%u (decode not implemented)",
             result.sampleRate, result.numChannels);
    } else {
        LOGW("OGG: could not find vorbis identification header");
    }

    return result;
}

// ============================================================================
// AudioDecoder - Auto-detect decode
// ============================================================================

AudioData AudioDecoder::decode(const uint8_t* fileData, size_t fileSize) {
    AudioFormat fmt = detectFormat(fileData, fileSize);

    switch (fmt) {
        case AudioFormat::WAV_PCM:
            return decodeWav(fileData, fileSize);
        case AudioFormat::MP3:
            LOGW("MP3 decode not implemented, returning header info only");
            return parseMp3Header(fileData, fileSize);
        case AudioFormat::OGG_VORBIS:
            LOGW("OGG Vorbis decode not implemented, returning header info only");
            return parseOggHeader(fileData, fileSize);
        default:
            LOGE("Unknown audio format");
            return AudioData();
    }
}

// ============================================================================
// AudioDecoder - Sample rate conversion (linear interpolation)
// ============================================================================

std::vector<int16_t> AudioDecoder::resample(const std::vector<int16_t>& input,
                                              uint32_t inputRate,
                                              uint32_t outputRate) {
    if (inputRate == outputRate || input.empty()) {
        return input;
    }

    float ratio = static_cast<float>(inputRate) / static_cast<float>(outputRate);
    size_t outputLen = static_cast<size_t>(static_cast<float>(input.size()) / ratio);
    std::vector<int16_t> output(outputLen);

    for (size_t i = 0; i < outputLen; ++i) {
        float srcPos = static_cast<float>(i) * ratio;
        size_t srcIdx = static_cast<size_t>(srcPos);
        float frac = srcPos - static_cast<float>(srcIdx);

        if (srcIdx + 1 < input.size()) {
            // Linear interpolation between two samples
            float sample = static_cast<float>(input[srcIdx]) * (1.0f - frac) +
                           static_cast<float>(input[srcIdx + 1]) * frac;
            // Clamp to int16 range
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            output[i] = static_cast<int16_t>(sample);
        } else if (srcIdx < input.size()) {
            output[i] = input[srcIdx];
        } else {
            output[i] = 0;
        }
    }

    LOGD("Resampled: %lu samples @ %uHz -> %lu samples @ %uHz",
         static_cast<unsigned long>(input.size()), inputRate,
         static_cast<unsigned long>(output.size()), outputRate);

    return output;
}

// ============================================================================
// AudioDecoder - Bit depth conversion
// ============================================================================

std::vector<int16_t> AudioDecoder::convert24to16(const uint8_t* src, size_t sampleCount) {
    std::vector<int16_t> output(sampleCount);

    for (size_t i = 0; i < sampleCount; ++i) {
        // Read 24-bit signed sample (little-endian)
        int32_t sample = static_cast<int32_t>(src[i * 3]) |
                         (static_cast<int32_t>(src[i * 3 + 1]) << 8) |
                         (static_cast<int32_t>(src[i * 3 + 2]) << 16);

        // Sign extend from 24-bit
        if (sample & 0x800000) {
            sample |= 0xFF000000;
        }

        // Shift down to 16-bit (divide by 256)
        output[i] = static_cast<int16_t>(sample >> 8);
    }

    return output;
}

std::vector<int16_t> AudioDecoder::convert8to16(const uint8_t* src, size_t sampleCount) {
    std::vector<int16_t> output(sampleCount);

    for (size_t i = 0; i < sampleCount; ++i) {
        // 8-bit unsigned -> 16-bit signed
        output[i] = static_cast<int16_t>((static_cast<int>(src[i]) - 128) * 256);
    }

    return output;
}

} // namespace audio
