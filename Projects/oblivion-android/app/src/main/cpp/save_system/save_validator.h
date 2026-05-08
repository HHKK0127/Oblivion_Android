#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace SaveSystem {

enum class ValidationError {
    OK,
    CORRUPTED_JSON,
    INVALID_VERSION,
    INVALID_DATA,
    CHECKSUM_MISMATCH,
    MISSING_FIELDS
};

class SaveValidator {
public:
    SaveValidator();
    ~SaveValidator();

    /**
     * Validate a JSON game state object
     * @param j The JSON object to validate
     * @return ValidationError indicating if validation passed or what failed
     */
    ValidationError validate(const json& j) const;

    /**
     * Get human-readable error message for a validation error
     * @param err The validation error enum
     * @return Error message string
     */
    std::string getErrorMessage(ValidationError err) const;

    // ====================================================================
    // Data Validation Helpers
    // ====================================================================

    /**
     * Check if health values are valid
     * @param health Current health (should be >= 0 and <= maxHealth)
     * @param maxHealth Maximum health (should be > 0)
     * @return true if valid, false otherwise
     */
    bool isHealthValid(float health, float maxHealth) const;

    /**
     * Check if mana values are valid
     * @param mana Current mana (should be >= 0 and <= maxMana)
     * @param maxMana Maximum mana (should be >= 0)
     * @return true if valid, false otherwise
     */
    bool isManaValid(float mana, float maxMana) const;

    /**
     * Check if stamina values are valid
     * @param stamina Current stamina (should be >= 0 and <= maxStamina)
     * @param maxStamina Maximum stamina (should be > 0)
     * @return true if valid, false otherwise
     */
    bool isStaminaValid(float stamina, float maxStamina) const;

    /**
     * Check if a quest state value is valid
     * @param state The quest state integer (0-4 = PENDING to FAILED)
     * @return true if valid, false otherwise
     */
    bool isQuestStateValid(int state) const;

    /**
     * Check if an attribute value is valid
     * @param value The attribute value (typically 0-100)
     * @return true if valid, false otherwise
     */
    bool isAttributeValid(float value) const;

    /**
     * Check if a skill value is valid
     * @param value The skill value (typically 0-100)
     * @return true if valid, false otherwise
     */
    bool isSkillValid(float value) const;

    // ====================================================================
    // Checksum/Hash Computation
    // ====================================================================

    /**
     * Compute a checksum for a JSON object
     * Uses simple CRC32 for performance on mobile
     * @param j The JSON object to checksum
     * @return Checksum as hex string
     */
    std::string computeChecksum(const json& j) const;

    /**
     * Verify a JSON object's checksum
     * @param j The JSON object with embedded checksum
     * @param expectedChecksum The expected checksum value
     * @return true if checksums match, false otherwise
     */
    bool verifyChecksum(const json& j, const std::string& expectedChecksum) const;

    /**
     * Add a checksum to a JSON object (modifies the object)
     * @param j The JSON object to add checksum to
     */
    void addChecksum(json& j) const;

private:
    // Version compatibility
    std::string SUPPORTED_VERSION = "0.6.0";

    // Validation constants
    static constexpr float MIN_ATTRIBUTE_VALUE = 0.0f;
    static constexpr float MAX_ATTRIBUTE_VALUE = 100.0f;
    static constexpr float MIN_SKILL_VALUE = 0.0f;
    static constexpr float MAX_SKILL_VALUE = 100.0f;
};

}  // namespace SaveSystem
