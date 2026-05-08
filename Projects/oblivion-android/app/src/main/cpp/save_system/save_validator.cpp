#include "save_validator.h"
#include <android/log.h>
#include <sstream>
#include <iomanip>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "SaveValidator", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "SaveValidator", __VA_ARGS__)

namespace SaveSystem {

SaveValidator::SaveValidator() {
    LOGI("SaveValidator initialized");
}

SaveValidator::~SaveValidator() {
}

// ============================================================================
// Main Validation Function
// ============================================================================

ValidationError SaveValidator::validate(const json& j) const {
    try {
        // Check if it's a valid JSON object
        if (!j.is_object()) {
            LOGE("Root element is not a JSON object");
            return ValidationError::CORRUPTED_JSON;
        }

        // Check for required top-level fields
        if (!j.contains("metadata")) {
            LOGE("Missing 'metadata' field");
            return ValidationError::MISSING_FIELDS;
        }

        const json& metadata = j["metadata"];

        // Validate version
        if (!metadata.contains("version")) {
            LOGE("Missing version field");
            return ValidationError::MISSING_FIELDS;
        }

        std::string version = metadata["version"];
        if (version != SUPPORTED_VERSION) {
            LOGE("Version mismatch: expected %s, got %s",
                 SUPPORTED_VERSION.c_str(), version.c_str());
            return ValidationError::INVALID_VERSION;
        }

        // Validate checksum if present
        if (metadata.contains("checksum")) {
            std::string expectedChecksum = metadata["checksum"];
            if (!verifyChecksum(j, expectedChecksum)) {
                LOGE("Checksum verification failed");
                return ValidationError::CHECKSUM_MISMATCH;
            }
        }

        // Check for character status validity if present
        if (j.contains("playerState")) {
            const json& playerState = j["playerState"];

            if (playerState.contains("status")) {
                const json& status = playerState["status"];

                float currentHealth = status.value("currentHealth", 100.0f);
                float maxHealth = status.value("maxHealth", 100.0f);
                if (!isHealthValid(currentHealth, maxHealth)) {
                    LOGE("Invalid health values: %.2f / %.2f", currentHealth, maxHealth);
                    return ValidationError::INVALID_DATA;
                }

                float currentMana = status.value("currentMana", 50.0f);
                float maxMana = status.value("maxMana", 50.0f);
                if (!isManaValid(currentMana, maxMana)) {
                    LOGE("Invalid mana values: %.2f / %.2f", currentMana, maxMana);
                    return ValidationError::INVALID_DATA;
                }

                float stamina = status.value("stamina", 100.0f);
                float maxStamina = status.value("maxStamina", 100.0f);
                if (!isStaminaValid(stamina, maxStamina)) {
                    LOGE("Invalid stamina values: %.2f / %.2f", stamina, maxStamina);
                    return ValidationError::INVALID_DATA;
                }

                // Validate attributes
                if (status.contains("attributes") && status["attributes"].is_object()) {
                    for (const auto& [key, value] : status["attributes"].items()) {
                        if (!isAttributeValid(value.get<float>())) {
                            LOGE("Invalid attribute %s: %.2f", key.c_str(), value.get<float>());
                            return ValidationError::INVALID_DATA;
                        }
                    }
                }

                // Validate skills
                if (status.contains("skills") && status["skills"].is_object()) {
                    for (const auto& [key, value] : status["skills"].items()) {
                        if (!isSkillValid(value.get<float>())) {
                            LOGE("Invalid skill %s: %.2f", key.c_str(), value.get<float>());
                            return ValidationError::INVALID_DATA;
                        }
                    }
                }
            }
        }

        // Validate quest states if present
        if (j.contains("questStates") && j["questStates"].is_object()) {
            for (const auto& [questId, state] : j["questStates"].items()) {
                if (state.is_number()) {
                    if (!isQuestStateValid(state.get<int>())) {
                        LOGE("Invalid quest state for quest %s: %d",
                             questId.c_str(), state.get<int>());
                        return ValidationError::INVALID_DATA;
                    }
                }
            }
        }

        LOGI("Validation passed");
        return ValidationError::OK;

    } catch (const json::exception& e) {
        LOGE("JSON parsing error: %s", e.what());
        return ValidationError::CORRUPTED_JSON;
    } catch (const std::exception& e) {
        LOGE("Unexpected error during validation: %s", e.what());
        return ValidationError::INVALID_DATA;
    }
}

// ============================================================================
// Error Messages
// ============================================================================

std::string SaveValidator::getErrorMessage(ValidationError err) const {
    switch (err) {
        case ValidationError::OK:
            return "Validation successful";
        case ValidationError::CORRUPTED_JSON:
            return "Save file is corrupted or invalid JSON";
        case ValidationError::INVALID_VERSION:
            return "Save file version is incompatible with current game version";
        case ValidationError::INVALID_DATA:
            return "Save file contains invalid data values";
        case ValidationError::CHECKSUM_MISMATCH:
            return "Save file checksum does not match - file may be corrupted";
        case ValidationError::MISSING_FIELDS:
            return "Save file is missing required fields";
        default:
            return "Unknown validation error";
    }
}

// ============================================================================
// Data Validation Helpers
// ============================================================================

bool SaveValidator::isHealthValid(float health, float maxHealth) const {
    return health >= 0.0f && health <= maxHealth && maxHealth > 0.0f;
}

bool SaveValidator::isManaValid(float mana, float maxMana) const {
    return mana >= 0.0f && mana <= maxMana && maxMana >= 0.0f;
}

bool SaveValidator::isStaminaValid(float stamina, float maxStamina) const {
    return stamina >= 0.0f && stamina <= maxStamina && maxStamina > 0.0f;
}

bool SaveValidator::isQuestStateValid(int state) const {
    // Valid values: 0 (PENDING) to 4 (FAILED)
    return state >= 0 && state <= 4;
}

bool SaveValidator::isAttributeValid(float value) const {
    return value >= MIN_ATTRIBUTE_VALUE && value <= MAX_ATTRIBUTE_VALUE;
}

bool SaveValidator::isSkillValid(float value) const {
    return value >= MIN_SKILL_VALUE && value <= MAX_SKILL_VALUE;
}

// ============================================================================
// Checksum Computation
// ============================================================================

// Simple CRC32 implementation for fast checksums
static uint32_t crc32_table[256];
static bool crc32_table_computed = false;

static void compute_crc32_table() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc = crc >> 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_computed = true;
}

static uint32_t crc32(uint32_t crc, const uint8_t* data, size_t len) {
    if (!crc32_table_computed) {
        compute_crc32_table();
    }

    crc = crc ^ 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

std::string SaveValidator::computeChecksum(const json& j) const {
    // Create a copy without the checksum field
    json copy = j;
    if (copy.contains("metadata") && copy["metadata"].contains("checksum")) {
        copy["metadata"].erase("checksum");
    }

    // Serialize to string
    std::string jsonStr = copy.dump();

    // Compute CRC32
    uint32_t crcValue = crc32(0,
        reinterpret_cast<const uint8_t*>(jsonStr.c_str()),
        jsonStr.length());

    // Convert to hex string
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << crcValue;
    return ss.str();
}

bool SaveValidator::verifyChecksum(const json& j, const std::string& expectedChecksum) const {
    std::string computed = computeChecksum(j);
    bool matches = computed == expectedChecksum;

    if (!matches) {
        LOGE("Checksum mismatch: expected %s, got %s",
             expectedChecksum.c_str(), computed.c_str());
    }

    return matches;
}

void SaveValidator::addChecksum(json& j) const {
    std::string checksum = computeChecksum(j);
    if (!j.contains("metadata")) {
        j["metadata"] = json::object();
    }
    j["metadata"]["checksum"] = checksum;
    LOGI("Added checksum: %s", checksum.c_str());
}

}  // namespace SaveSystem
