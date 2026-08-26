#include "book_reader.h"
#include <android/log.h>

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
        // Find the skill name by formID
        // For now, we'll use a generic skill increase
        // In a full implementation, we'd map formID to skill name
        LOGD("Skill book: teaches skill 0x%08X level %u",
             book->teachesSkillID, book->teachesSkillLevel);

        // Apply skill increase to a random skill for now
        // TODO: Map skill formID to skill name
        for (auto& skill : playerStatus.skills) {
            if (skill.second < 100.0f) {
                skill.second += static_cast<float>(book->teachesSkillLevel);
                if (skill.second > 100.0f) skill.second = 100.0f;
                LOGD("Increased skill %s by %u (now %.0f)",
                     skill.first.c_str(), book->teachesSkillLevel, skill.second);
                break;
            }
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

} // namespace oblivion
