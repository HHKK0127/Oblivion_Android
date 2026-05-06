#include "book_database.h"
#include <cstdlib>
#include <android/log.h>

#define LOG_TAG "BookDatabase"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

BookDatabase::BookDatabase() {
    LOGD("BookDatabase created");
}

BookDatabase::~BookDatabase() {
    books.clear();
    LOGD("BookDatabase destroyed");
}

void BookDatabase::initialize() {
    LOGI("BookDatabase initializing with 10 Oblivion-inspired books...");

    // Book 1: The Elder Scrolls - Prophecy/Lore (3 pages)
    registerBook(Book(1, "book_elder_scrolls_title",
        {"book_elder_scrolls_page1", "book_elder_scrolls_page2", "book_elder_scrolls_page3"},
        "Various", 0, 100));

    // Book 2: Nerevar Moon-and-Star - Historical Epic (4 pages)
    registerBook(Book(2, "book_nerevar_title",
        {"book_nerevar_page1", "book_nerevar_page2", "book_nerevar_page3", "book_nerevar_page4"},
        "Vivec", 2, 150));

    // Book 3: Song of the Tenpenny Towers - Comedic Poetry (2 pages)
    registerBook(Book(3, "book_tenpenny_title",
        {"book_tenpenny_page1", "book_tenpenny_page2"},
        "Bard's Tavern", 0, 30));

    // Book 4: A Dance in Fire - Action/Adventure (3 pages)
    registerBook(Book(4, "book_dance_fire_title",
        {"book_dance_fire_page1", "book_dance_fire_page2", "book_dance_fire_page3"},
        "Khajiit Scholar", 1, 80));

    // Book 5: Beggar - Philosophy (2 pages)
    registerBook(Book(5, "book_beggar_title",
        {"book_beggar_page1", "book_beggar_page2"},
        "Anonymous", 0, 25));

    // Book 6: The Refugees - Tragedy (2 pages)
    registerBook(Book(6, "book_refugees_title",
        {"book_refugees_page1", "book_refugees_page2"},
        "Wary Wanderer", 0, 40));

    // Book 7: Mysticism Spellcraft - Magic Reference (3 pages)
    registerBook(Book(7, "book_mysticism_title",
        {"book_mysticism_page1", "book_mysticism_page2", "book_mysticism_page3"},
        "Mage's Guild", 3, 120));

    // Book 8: Arboretum - Nature Guide (2 pages)
    registerBook(Book(8, "book_arboretum_title",
        {"book_arboretum_page1", "book_arboretum_page2"},
        "Botanist", 1, 50));

    // Book 9: Legend of Pelinal - Creation Myth (3 pages)
    registerBook(Book(9, "book_pelinal_title",
        {"book_pelinal_page1", "book_pelinal_page2", "book_pelinal_page3"},
        "Elder Chantry", 2, 90));

    // Book 10: Immortal Blood - Vampire Story (2 pages)
    registerBook(Book(10, "book_blood_title",
        {"book_blood_page1", "book_blood_page2"},
        "Dark Author", 0, 60));

    LOGI("BookDatabase initialized with %zu books (total %d pages)",
         books.size(), 24);
}

void BookDatabase::registerBook(const Book& book) {
    if (books.find(book.bookId) != books.end()) {
        LOGD("Book %u already exists, skipping", book.bookId);
        return;
    }
    books[book.bookId] = book;
    LOGD("Registered book: %u (%s) with %d pages, author: %s, value: %u gold",
         book.bookId, book.titleKey.c_str(), book.getPageCount(), book.author.c_str(), book.value);
}

const Book* BookDatabase::getBook(uint32_t bookId) const {
    auto it = books.find(bookId);
    if (it == books.end()) {
        LOGD("Book %u not found in database", bookId);
        return nullptr;
    }
    return &it->second;
}

const std::vector<uint32_t> BookDatabase::getAllBookIds() const {
    std::vector<uint32_t> ids;
    for (const auto& pair : books) {
        ids.push_back(pair.first);
    }
    return ids;
}

uint32_t BookDatabase::getRandomBookId() const {
    if (books.empty()) {
        LOGD("BookDatabase is empty, cannot get random book");
        return 0;
    }
    auto ids = getAllBookIds();
    uint32_t randomId = ids[rand() % ids.size()];
    LOGD("Random book selected: %u", randomId);
    return randomId;
}
