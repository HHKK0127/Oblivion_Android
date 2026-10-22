#pragma once

#include <cstdint>
#include <cstring>
#include <istream>
#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG_VER "SaveFormatVersion"
#define VER_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_VER, __VA_ARGS__)
#define VER_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_VER, __VA_ARGS__)

// ============================================================================
// Save Format Version Management
// Magic number + version checking + migration support
// ============================================================================

namespace save_format {

// Magic number: "OBLV" in ASCII
constexpr uint32_t MAGIC_NUMBER = 0x4F424C56;

// Current save format version
// v1: original binary layout
// v2: player magicka/maxMagicka appended to the PLYR block (after maxHealth)
// v3: playerLevel + playTimeSeconds appended to the 64-byte header (see
//     LEGACY_HEADER_SIZE); the payload is unchanged from v2
// v4: SCRP payload holds script global variables and a new CNTR section
//     (persisted container inventories) is appended after SCRP
constexpr uint32_t CURRENT_VERSION = 4;

// Minimum supported version (for backward compatibility)
constexpr uint32_t MIN_SUPPORTED_VERSION = 1;

// Size of the v1/v2 header. v3 appends two uint32 fields after slotName.
// Readers must consume this prefix first so v1/v2 saves still load.
constexpr uint32_t LEGACY_HEADER_SIZE = 64;

// Save file header (written at the start of every save file)
// Packed to ensure consistent binary layout across platforms
#pragma pack(push, 1)
struct SaveHeader {
    uint32_t magic;           // Must be MAGIC_NUMBER (4)
    uint32_t formatVersion;   // Format version (4)
    uint32_t gameVersion;     // Game version (4)
    uint64_t timestamp;       // Unix timestamp of save (8)
    uint32_t checksum;        // CRC32 of payload (4)
    uint32_t payloadSize;     // Size of payload in bytes (4)
    uint32_t slotIndex;       // Save slot index (0=auto, 1-10=manual) (4)
    char slotName[32];        // Human-readable slot name (32) -> total 64
    uint32_t playerLevel;     // v3+: player level at save time (4)
    uint32_t playTimeSeconds; // v3+: accumulated play time in seconds (4) -> 72

    SaveHeader()
        : magic(MAGIC_NUMBER), formatVersion(CURRENT_VERSION),
          gameVersion(0x000700), timestamp(0), checksum(0),
          payloadSize(0), slotIndex(0), playerLevel(1), playTimeSeconds(0) {
        std::memset(slotName, 0, sizeof(slotName));
    }
};
#pragma pack(pop)

static_assert(sizeof(SaveHeader) == LEGACY_HEADER_SIZE + 8,
              "SaveHeader must be 72 bytes (64-byte legacy prefix + v3 fields)");

// Reads a save header from an open stream, tolerating the shorter v1/v2 layout.
// The v1/v2 fields occupy the first 64 bytes, so reading that prefix is safe.
inline bool readSaveHeader(std::istream& in, SaveHeader& out) {
    out = SaveHeader{};
    in.read(reinterpret_cast<char*>(&out), LEGACY_HEADER_SIZE);
    if (!in) {
        return false;
    }
    if (out.formatVersion >= 3) {
        in.read(reinterpret_cast<char*>(&out.playerLevel),
                sizeof(out.playerLevel) + sizeof(out.playTimeSeconds));
        if (!in) {
            return false;
        }
    } else {
        out.playerLevel = 1;
        out.playTimeSeconds = 0;
    }
    return true;
}

// ============================================================================
// Version validation
// ============================================================================

enum class ValidationResult {
    OK,
    INVALID_MAGIC,
    UNSUPPORTED_VERSION,
    CORRUPTED_CHECKSUM,
    TOO_NEW
};

inline ValidationResult validateHeader(const SaveHeader& header) {
    if (header.magic != MAGIC_NUMBER) {
        VER_LOGE("Invalid magic number: 0x%08X (expected 0x%08X)",
                 header.magic, MAGIC_NUMBER);
        return ValidationResult::INVALID_MAGIC;
    }

    if (header.formatVersion < MIN_SUPPORTED_VERSION) {
        VER_LOGE("Save version too old: %u (min supported: %u)",
                 header.formatVersion, MIN_SUPPORTED_VERSION);
        return ValidationResult::UNSUPPORTED_VERSION;
    }

    if (header.formatVersion > CURRENT_VERSION) {
        VER_LOGE("Save version too new: %u (current: %u)",
                 header.formatVersion, CURRENT_VERSION);
        return ValidationResult::TOO_NEW;
    }

    return ValidationResult::OK;
}

// ============================================================================
// Checksum (simple CRC32)
// ============================================================================

inline uint32_t computeCRC32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

// ============================================================================
// Migration support
// ============================================================================

struct MigrationResult {
    bool success = false;
    uint32_t fromVersion = 0;
    uint32_t toVersion = 0;
    std::string errorMessage;
};

// Placeholder for future migrations
inline MigrationResult migrate(uint32_t fromVersion, uint32_t toVersion,
                                const uint8_t* data, size_t dataSize,
                                std::vector<uint8_t>& outData) {
    MigrationResult result;
    result.fromVersion = fromVersion;
    result.toVersion = toVersion;

    // Currently only version 1 exists, no migration needed
    if (fromVersion == toVersion) {
        outData.assign(data, data + dataSize);
        result.success = true;
        return result;
    }

    // Future: add migration paths here
    // if (fromVersion == 1 && toVersion == 2) { ... }

    result.errorMessage = "No migration path from v" +
                          std::to_string(fromVersion) + " to v" +
                          std::to_string(toVersion);
    VER_LOGE("%s", result.errorMessage.c_str());
    return result;
}

} // namespace save_format
