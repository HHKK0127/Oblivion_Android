#include "book_reader.h"
#include <android/log.h>
#include <unordered_map>

#define LOG_TAG "BookReader"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

BookReader::BookReader() {
}

BookReader::~BookReader() {
}

void BookReader::initialize(const ESMManager* esmMgr) {
    esmManager = esmMgr;
    LOGI("BookReader initialized");
}

bool BookReader::readBook(uint32_t bookFormID, CharacterStatus& playerStatus) {
    if (!esmManager) {
        LOGW("BookReader not initialized");
        return false;
    }

    const BookData* book = esmManager->findBook(bookFormID);
    if (!book) {
        LOGW("Book not found: 0x%08X", bookFormID);
        return false;
    }

    LOGD("Reading book: %s", book->fullName.c_str());

    // Check if this is a skill book
    if (book->teachesSkillID != 0 && book->teachesSkillLevel > 0) {
        LOGD("Skill book: teaches skill 0x%08X level %u",
             book->teachesSkillID, book->teachesSkillLevel);

        // BUG FIX #69: Resolve skill formID to actual skill name using ESM data
        std::string skillName = resolveSkillName(book->teachesSkillID);

        if (skillName.empty()) {
            LOGW("Could not resolve skill formID 0x%08X to skill name", book->teachesSkillID);
            return false;
        }

        // BUG FIX #70 & #71: Find the specific skill in player's skill map
        auto it = playerStatus.skills.find(skillName);
        if (it != playerStatus.skills.end()) {
            if (it->second < 100.0f) {
                it->second += static_cast<float>(book->teachesSkillLevel);
                if (it->second > 100.0f) it->second = 100.0f;
                LOGD("Increased skill %s by %u (now %.0f)",
                     skillName.c_str(), book->teachesSkillLevel, it->second);
            } else {
                LOGD("Skill %s already at max (100)", skillName.c_str());
            }
        } else {
            // BUG FIX #70: If player doesn't have this skill yet, add it
            playerStatus.skills[skillName] = static_cast<float>(book->teachesSkillLevel);
            LOGD("Added new skill %s at level %u", skillName.c_str(), book->teachesSkillLevel);
        }
    }

    return true;
}

bool BookReader::isSkillBook(uint32_t bookFormID) const {
    if (!esmManager) return false;

    const BookData* book = esmManager->findBook(bookFormID);
    if (!book) return false;

    return book->teachesSkillID != 0 && book->teachesSkillLevel > 0;
}

std::string BookReader::getBookDescription(uint32_t bookFormID) const {
    if (!esmManager) return "";

    const BookData* book = esmManager->findBook(bookFormID);
    if (!book) return "";

    return book->description;
}

std::string BookReader::resolveSkillName(uint32_t skillFormID) const {
    if (!esmManager) return "";

    // BUG FIX #69: Look up the skill in ESM data by formID
    const SkillData* skill = esmManager->findSkill(skillFormID);
    if (skill) {
        LOGD("Resolved skill formID 0x%08X to '%s' (skillID=%u)",
             skillFormID, skill->fullName.c_str(), skill->skillID);
        return skill->fullName;
    }

    // Fallback: Oblivion skill formIDs are in range 0x0000044C - 0x00000460
    // Map them to the skill names used in CharacterStatus
    static const std::unordered_map<uint32_t, std::string> skillFormIDMap = {
        {0x0000044C, "Blade"},
        {0x0000044D, "Blunt"},
        {0x0000044E, "HandToHand"},
        {0x0000044F, "Armorer"},
        {0x00000450, "Block"},
        {0x00000451, "HeavyArmor"},
        {0x00000452, "Alchemy"},
        {0x00000453, "Alteration"},
        {0x00000454, "Conjuration"},
        {0x00000455, "Destruction"},
        {0x00000456, "Illusion"},
        {0x00000457, "Mysticism"},
        {0x00000458, "Restoration"},
        {0x00000459, "Acrobatics"},
        {0x0000045A, "LightArmor"},
        {0x0000045B, "Marksman"},
        {0x0000045C, "Mercantile"},
        {0x0000045D, "Security"},
        {0x0000045E, "Sneak"},
        {0x0000045F, "Speechcraft"},
        {0x00000460, "Athletics"},
    };

    auto it = skillFormIDMap.find(skillFormID);
    if (it != skillFormIDMap.end()) {
        LOGD("Resolved skill formID 0x%08X to '%s' via fallback map", skillFormID, it->second.c_str());
        return it->second;
    }

    LOGW("Could not resolve skill formID 0x%08X", skillFormID);
    return "";
}

} // namespace oblivion
