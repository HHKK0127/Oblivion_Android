#pragma once

#include "book.h"
#include <unordered_map>
#include <vector>

class BookDatabase {
private:
    std::unordered_map<uint32_t, Book> books;
    void registerBook(const Book& book);

public:
    BookDatabase();
    ~BookDatabase();

    void initialize();
    const Book* getBook(uint32_t bookId) const;
    std::vector<uint32_t> getAllBookIds() const;
    uint32_t getRandomBookId() const;
    int getBookCount() const { return static_cast<int>(books.size()); }
};
