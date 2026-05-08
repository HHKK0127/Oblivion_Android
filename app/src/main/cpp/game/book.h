#pragma once

#include <string>
#include <cstdint>
#include <vector>

/**
 * @brief Book Content Structure
 * Stores book metadata and page content keys for localization
 *
 * Books are collectible items that players can read.
 * Content is stored as localization keys to support multiple languages.
 */
struct Book {
    uint32_t bookId;                  // Unique identifier (1-100)
    std::string titleKey;             // Localization key for title (e.g., "book_elder_scrolls_title")
    std::vector<std::string> pageKeys; // Localization keys for pages (e.g., {"book_elder_scrolls_page1", ...})

    std::string author;               // Book author (for flavor/metadata)
    int skillBonus;                   // Skill points gained from reading (0 = none)
    float weight;                     // Weight in kg (typically 0.1 for books)
    uint32_t value;                   // Gold value (20-200 typical)

    /**
     * @brief Default constructor
     */
    Book() : bookId(0), skillBonus(0), weight(0.1f), value(50) {}

    /**
     * @brief Parameterized constructor
     * @param id Unique book identifier
     * @param tKey Localization key for title
     * @param pKeys Localization keys for each page
     * @param auth Author name
     * @param skill Skill points bonus (0 = none)
     * @param val Gold value
     */
    Book(uint32_t id, const std::string& tKey, const std::vector<std::string>& pKeys,
         const std::string& auth = "Unknown", int skill = 0, uint32_t val = 50)
        : bookId(id), titleKey(tKey), pageKeys(pKeys), author(auth),
          skillBonus(skill), weight(0.1f), value(val) {}

    /**
     * @brief Get the number of pages in this book
     * @return Number of pages (length of pageKeys vector)
     */
    int getPageCount() const { return pageKeys.size(); }
};
