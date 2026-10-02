#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class AssetManager;

/**
 * @brief In-game voice line index built from the BSA voice archives.
 *
 * Oblivion voice files live inside Voices1.bsa / Voices2.bsa under
 * sound/voice/oblivion.esm/{race}/{gender}/{prefix}_{formID:08x}_{n}.mp3.
 * The ESM INFO records carry no EDID/SNDD/SNAM, so a voice line is instead
 * matched by its INFO FormID and the 1-based response ordinal (TRDT[12]),
 * e.g. the formID 0x0018BD5D response 1 becomes key "0018bd5d_1".
 *
 * Builds a {formID:08x}_{n} -> BSA-internal path index once at startup and
 * serves voice path lookups for dialogue playback.
 */
class VoiceSoundDatabase {
public:
    /**
     * @brief Scan the loaded BSA archives and index all voice lines.
     * @param assetManager AssetManager with Voices1/2.bsa already loaded
     * @return Number of indexed voice files
     */
    size_t initialize(AssetManager* assetManager);

    /**
     * @brief Resolve an INFO formID + response ordinal to a BSA-internal path.
     * @param formID INFO record FormID
     * @param responseNumber 1-based response ordinal (TRDT[12])
     * @return BSA-internal path (e.g. sound/voice/...) or empty when no match
     */
    std::string resolveVoicePath(uint32_t formID, uint8_t responseNumber) const;

    /**
     * @brief Number of indexed voice files.
     */
    size_t getVoiceCount() const { return m_voiceCount; }

    /**
     * @brief Number of distinct formID/response keys.
     */
    size_t getKeyCount() const { return m_voiceIndex.size(); }

    /**
     * @brief Extract the "{formID:08x}_{n}" key from a lowercased BSA path.
     * @param lowerPath Lowercased BSA-internal path (e.g. sound/voice/...)
     * @param responseNumber Out: 1-based response ordinal
     * @return Index key, or empty when the path has no 8-hex formID segment
     */
    static std::string extractFormIdKey(const std::string& lowerPath,
                                        uint8_t& responseNumber);

private:
    // Key "{formID:08x}_{n}" -> matching BSA-internal paths (usually one).
    std::unordered_map<std::string, std::vector<std::string>> m_voiceIndex;
    size_t m_voiceCount = 0;
};