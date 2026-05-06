#pragma once

#include "book.h"
#include <unordered_map>
#include <vector>
#include <memory>

/**
 * @brief Book Database Manager
 * Manages all books available in the game.
 * Stores book metadata and provides retrieval functions.
 *
 * Usage:
 *   BookDatabase db;
 *   db.initialize();
 *   const Book* book = db.getBook(1);  // Get book by ID
 */
class BookDatabase {
private:
    // Database of all books, keyed by bookId
    std::unordered_map<uint32_t, Book> books;

    /**
     * @brief Register a book into the database
     * @param book The book to register
     */
    void registerBook(const Book& book);

public:
    /**
     * @brief Constructor
     */
    BookDatabase();

    /**
     * @brief Destructor
     */
    ~BookDatabase();

    /**
     * @brief Initialize the database with all available books
     * Populates the books map with 10 Oblivion-inspired books.
     */
    void initialize();

    /**
     * @brief Retrieve a book by its ID
     * @param bookId The ID of the book to retrieve
     * @return Pointer to the book, or nullptr if not found
     */
    const Book* getBook(uint32_t bookId) const;

    /**
     * @brief Get all book IDs in the database
     * @return Vector of all book IDs
     */
    const std::vector<uint32_t> getAllBookIds() const;

    /**
     * @brief Get a random book ID for loot generation
     * @return Random book ID from the database
     */
    uint32_t getRandomBookId() const;

    /**
     * @brief Get the total number of books in the database
     * @return Number of books
     */
    int getBookCount() const { return books.size(); }

    /**
     * @brief Check if a book exists
     * @param bookId The ID to check
     * @return True if book exists, false otherwise
     */
    bool bookExists(uint32_t bookId) const { return books.find(bookId) != books.end(); }
};
