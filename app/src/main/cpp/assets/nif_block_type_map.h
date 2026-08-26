#pragma once

#include "nif_types.h"
#include <string>
#include <unordered_map>
#include <array>

// String-based NIF block type identification
// Oblivion NIF (ver 20.0.0.x) uses block type strings, not numeric IDs
class NIFBlockTypeMap {
public:
    static NIFBlockType fromString(const std::string& name);
    static const char* toString(NIFBlockType type);

private:
    static const std::unordered_map<std::string, NIFBlockType> stringToType;
    static const std::array<const char*, static_cast<size_t>(NIFBlockType::NumTypes)> typeToString;
};
