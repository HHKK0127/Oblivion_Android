#pragma once

#include <string>
#include <cstdint>
#include <vector>

struct Book {
    uint32_t bookId;
    std::string titleKey;
    std::vector<std::string> pageKeys;
    std::string author;
    int skillBonus;
    float weight;
    uint32_t value;

    Book(uint32_t id, const std::string& title, const std::vector<std::string>& pages,
         const std::string& auth, int skill, float w, uint32_t val)
        : bookId(id), titleKey(title), pageKeys(pages), author(auth),
          skillBonus(skill), weight(w), value(val) {}

    int getPageCount() const { return static_cast<int>(pageKeys.size()); }
};
