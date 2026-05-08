#include "book_database.h"
#include <android/log.h>
#include <random>

#define LOG_TAG_BOOK "BookDatabase"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_BOOK, __VA_ARGS__)

BookDatabase::BookDatabase() {}

BookDatabase::~BookDatabase() {}

void BookDatabase::registerBook(const Book& book) {
    books[book.bookId] = book;
}

void BookDatabase::initialize() {
    // Book 1: The Elder Scrolls (3 pages)
    registerBook(Book(1, "book_elder_scrolls_title",
        {"book_elder_scrolls_page1", "book_elder_scrolls_page2", "book_elder_scrolls_page3"},
        "Various", 0, 0.5f, 100));

    // Book 2: Nerevar Moon-and-Star (4 pages)
    registerBook(Book(2, "book_nerevar_title",
        {"book_nerevar_page1", "book_nerevar_page2", "book_nerevar_page3", "book_nerevar_page4"},
        "Temple Historian", 0, 0.6f, 150));

    // Book 3: Song of the Tenpenny Towers (2 pages)
    registerBook(Book(3, "book_tenpenny_title",
        {"book_tenpenny_page1", "book_tenpenny_page2"},
        "Anonymous Bard", 0, 0.3f, 50));

    // Book 4: A Dance in Fire (3 pages)
    registerBook(Book(4, "book_dance_fire_title",
        {"book_dance_fire_page1", "book_dance_fire_page2", "book_dance_fire_page3"},
        "Waughin Jarth", 0, 0.5f, 80));

    // Book 5: Beggar (2 pages)
    registerBook(Book(5, "book_beggar_title",
        {"book_beggar_page1", "book_beggar_page2"},
        "Tevrens Torkel", 0, 0.3f, 40));

    // Book 6: The Refugees (2 pages)
    registerBook(Book(6, "book_refugees_title",
        {"book_refugees_page1", "book_refugees_page2"},
        "Gorgic Guine", 0, 0.4f, 60));

    // Book 7: Mysticism Spellcraft (3 pages)
    registerBook(Book(7, "book_mysticism_title",
        {"book_mysticism_page1", "book_mysticism_page2", "book_mysticism_page3"},
        "Archmagister Voth", 1, 0.7f, 200));

    // Book 8: Arboretum (2 pages)
    registerBook(Book(8, "book_arboretum_title",
        {"book_arboretum_page1", "book_arboretum_page2"},
        "Greenwarden Leral", 0, 0.4f, 55));

    // Book 9: Legend of Pelinal (3 pages)
    registerBook(Book(9, "book_pelinal_title",
        {"book_pelinal_page1", "book_pelinal_page2", "book_pelinal_page3"},
        "Adabal-a", 0, 0.5f, 90));

    // Book 10: Immortal Blood (2 pages)
    registerBook(Book(10, "book_immortal_blood_title",
        {"book_immortal_blood_page1", "book_immortal_blood_page2"},
        "Anonymous", 0, 0.4f, 75));

    LOGI("BookDatabase initialized with %zu books", books.size());
}

const Book* BookDatabase::getBook(uint32_t bookId) const {
    auto it = books.find(bookId);
    if (it != books.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<uint32_t> BookDatabase::getAllBookIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, book] : books) {
        ids.push_back(id);
    }
    return ids;
}

uint32_t BookDatabase::getRandomBookId() const {
    if (books.empty()) return 0;

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, books.size() - 1);

    auto it = books.begin();
    std::advance(it, dist(gen));
    return it->first;
}
