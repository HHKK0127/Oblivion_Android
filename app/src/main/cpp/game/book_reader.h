#pragma once

#include <cstdint>
#include <string>
#include "../assets/esm_reader.h"
#include "../localization/localization_manager.h"
#include "npc.h"

namespace oblivion {

/**
 * @brief Book reading system
 *
 * Handles reading books and applying skill bonuses for skill books.
 */
class BookReader {
public:
    BookReader();
    ~BookReader();

    /**
     * @brief Initialize with ESM manager
     * @param esmMgr ESM manager with loaded data
     */
    void initialize(const ESMManager* esmMgr);

    /**
     * @brief Set the localization source used for book text
     * @param localization Localization manager, or nullptr to disable
     */
    void setLocalizationManager(const LocalizationManager* localization) {
        localizationManager = localization;
    }

    /**
     * @brief Read a book and apply effects
     * @param bookFormID FormID of the book to read
     * @param playerStatus Player's character status to modify
     * @return true if book was read successfully
     */
    bool readBook(uint32_t bookFormID, CharacterStatus& playerStatus);

    /**
     * @brief Check if a book teaches a skill
     * @param bookFormID FormID of the book
     * @return true if the book is a skill book
     */
    bool isSkillBook(uint32_t bookFormID) const;

    /**
     * @brief Get book description
     * @param bookFormID FormID of the book
     * @return Book description text, or empty if not found
     */
    std::string getBookDescription(uint32_t bookFormID) const;

private:
    const ESMManager* esmManager = nullptr;
    const LocalizationManager* localizationManager = nullptr;

    /**
     * @brief Resolve a skill formID to a skill name
     * @param skillFormID The skill formID from the book
     * @return Skill name string, or empty if not found
     */
    std::string resolveSkillName(uint32_t skillFormID) const;
};

} // namespace oblivion
