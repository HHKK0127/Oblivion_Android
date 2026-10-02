#include "voice_sound_db.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <android/log.h>

#include "../assets/asset_manager.h"

#define LOG_TAG "VoiceSoundDB"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// Voice files live in sound/voice/... inside the BSA archives.
constexpr const char* kVoiceRoot = "sound/voice/";
}  // namespace

std::string VoiceSoundDatabase::extractFormIdKey(const std::string& rawPath,
                                                 uint8_t& responseNumber) {
    responseNumber = 0;

    // BSA archives report paths with backslash separators, while the voice
    // naming convention uses forward slashes. Normalize before matching.
    std::string lowerPath = rawPath;
    for (char& ch : lowerPath) {
        if (ch == '\\') {
            ch = '/';
        }
    }

    const std::string base = "oblivion.esm/";
    size_t pos = lowerPath.find(base);
    // Voice files may also live under their race/gender dirs, e.g.
    // sound/voice/oblivion.esm/argonian/f/...
    size_t seg = pos == std::string::npos
                     ? lowerPath.find(kVoiceRoot)
                     : pos + base.length();
    if (seg == std::string::npos) {
        return "";
    }

    // Walk the remaining segments; the last one is the file name.
    size_t slash = lowerPath.find('/', seg);
    while (slash != std::string::npos) {
        seg = slash + 1;
        slash = lowerPath.find('/', seg);
    }
    if (seg >= lowerPath.length()) {
        return "";
    }

    // File name must be "{prefix}_{formID:08x}_{n}.mp3".
    size_t ext = lowerPath.find(".mp3", seg);
    if (ext == std::string::npos) {
        return "";
    }

    std::string name = lowerPath.substr(seg, ext - seg);
    size_t lastUnderscore = name.rfind('_');
    if (lastUnderscore == std::string::npos) {
        return "";
    }
    size_t hexStart = name.rfind('_', lastUnderscore - 1);
    if (hexStart == std::string::npos) {
        return "";
    }
    size_t formIdLen = lastUnderscore - hexStart - 1;
    if (formIdLen != 8) {
        return "";
    }

    // Validate the hex digits and the trailing response ordinal.
    const std::string hexPart = name.substr(hexStart + 1, formIdLen);
    for (char c : hexPart) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            return "";
        }
    }
    const std::string nPart = name.substr(lastUnderscore + 1);
    if (nPart.empty() || nPart.length() > 3) {
        return "";
    }
    for (char c : nPart) {
        if (c < '0' || c > '9') {
            return "";
        }
    }
    responseNumber = static_cast<uint8_t>(std::atoi(nPart.c_str()));
    if (responseNumber == 0) {
        return "";
    }

    return name.substr(hexStart + 1);  // "{formID:08x}_{n}"
}

size_t VoiceSoundDatabase::initialize(AssetManager* assetManager) {
    m_voiceIndex.clear();
    m_voiceCount = 0;

    if (!assetManager) {
        LOGE("VoiceSoundDatabase: no AssetManager");
        return 0;
    }

    const std::vector<std::string> paths =
            assetManager->findFiles(kVoiceRoot);
        if (paths.empty()) {
            LOGW("VoiceSoundDatabase: no voice files under %s", kVoiceRoot);
            return 0;
        }

    for (const std::string& path : paths) {
        // findFiles returns the raw archive paths (mixed case); normalize to
        // lowercase like the BSA lookup table.
        std::string lower = path;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        uint8_t responseNumber = 0;
        const std::string key = extractFormIdKey(lower, responseNumber);
        if (key.empty()) {
            continue;
        }
        m_voiceIndex[key].push_back(path);
        ++m_voiceCount;
    }

    LOGI("VoiceSoundDatabase: indexed %zu voice files (%zu keys)",
         m_voiceCount, m_voiceIndex.size());
    return m_voiceCount;
}

std::string VoiceSoundDatabase::resolveVoicePath(
    uint32_t formID, uint8_t responseNumber) const {
    if (responseNumber == 0) {
        return "";
    }

    char key[16];
    std::snprintf(key, sizeof(key), "%08x_%u", formID, responseNumber);

    auto it = m_voiceIndex.find(key);
    if (it == m_voiceIndex.end()) {
        return "";
    }
    // Use the first match; a matching race/gender variant plays if present.
    return it->second.empty() ? "" : it->second.front();
}