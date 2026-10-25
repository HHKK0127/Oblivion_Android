#include "oblivion_ini.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace oblivion {
namespace {

char ascii_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

std::string to_lower(const char* text) {
    std::string out(text != nullptr ? text : "");
    for (char& c : out) {
        c = ascii_lower(c);
    }
    return out;
}

std::string trim(const std::string& text) {
    const std::size_t begin = text.find_first_not_of(" \t\r\n\f\v");
    if (begin == std::string::npos) {
        return std::string();
    }
    const std::size_t end = text.find_last_not_of(" \t\r\n\f\v");
    return text.substr(begin, end - begin + 1);
}

bool iequals(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; ++i) {
        if (ascii_lower(a[i]) != ascii_lower(b[i])) {
            return false;
        }
    }
    return i == a.size() && b[i] == '\0';
}

int digit_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/// The INI writes DirectInput scan codes as bare hex without an 0x prefix
/// (Forward=0011FFFF), so a digit string containing a-f is hex, not decimal.
bool looks_bare_hex(const std::string& text) {
    if (text.size() < 2) {
        return false;
    }
    bool letter = false;
    for (char c : text) {
        const int d = digit_value(c);
        if (d < 0) {
            return false;
        }
        if (d >= 10) {
            letter = true;
        }
    }
    return letter;
}

bool parse_int(const char* text, long long& out) {
    if (text == nullptr) {
        return false;
    }
    const char* p = text;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    bool negative = false;
    if (*p == '+' || *p == '-') {
        negative = (*p == '-');
        ++p;
    }
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        base = 16;
        p += 2;
    } else if (!negative && looks_bare_hex(std::string(p))) {
        base = 16;
    }
    if (*p == '\0') {
        return false;
    }
    long long value = 0;
    for (; *p != '\0'; ++p) {
        const int d = digit_value(*p);
        if (d < 0 || d >= base) {
            return false;
        }
        value = value * base + d;
        if (value > 0xFFFFFFFFLL) {
            return false;
        }
    }
    out = negative ? -value : value;
    return true;
}

bool parse_double(const char* text, double& out) {
    if (text == nullptr || *text == '\0') {
        return false;
    }
    char* end = nullptr;
    const double value = std::strtod(text, &end);
    if (end == text || end == nullptr || *end != '\0') {
        return false;
    }
    out = value;
    return true;
}

unsigned char clamp_byte(long long value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return static_cast<unsigned char>(value);
}

bool parse_color_triplet(const std::string& text, unsigned char* rgb) {
    long long channel[3] = {0, 0, 0};
    std::size_t pos = 0;
    for (int i = 0; i < 3; ++i) {
        const std::size_t comma = text.find(',', pos);
        const bool last = (i == 2);
        if (last != (comma == std::string::npos)) {
            return false;  // too few or too many fields
        }
        const std::size_t stop = (comma == std::string::npos) ? text.size() : comma;
        const std::string field = trim(text.substr(pos, stop - pos));
        if (!parse_int(field.c_str(), channel[i])) {
            return false;
        }
        pos = stop + 1;
    }
    for (int i = 0; i < 3; ++i) {
        rgb[i] = clamp_byte(channel[i]);
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------- default table

namespace ini_defaults {

namespace {

/// kDefaults is sorted by (lowercase section, lowercase key).
bool entry_less(const IniDefaultEntry& entry, const std::string& section,
                const std::string& key) {
    const std::string entry_section = to_lower(entry.section);
    if (entry_section != section) {
        return entry_section < section;
    }
    return to_lower(entry.key) < key;
}

}  // namespace

const IniDefaultEntry* find(const char* section, const char* key) {
    if (key == nullptr || *key == '\0') {
        return nullptr;
    }
    const std::string wanted_section = to_lower(section);
    const std::string wanted_key = to_lower(key);
    std::size_t low = 0;
    std::size_t high = kDefaultCount;
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (entry_less(kDefaults[mid], wanted_section, wanted_key)) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    if (low >= kDefaultCount) {
        return nullptr;
    }
    const IniDefaultEntry& candidate = kDefaults[low];
    if (to_lower(candidate.section) != wanted_section ||
        to_lower(candidate.key) != wanted_key) {
        return nullptr;
    }
    return &candidate;
}

const IniDefaultEntry* find_by_key(const char* key) {
    if (key == nullptr || *key == '\0') {
        return nullptr;
    }
    const std::string wanted = to_lower(key);
    for (std::size_t i = 0; i < kDefaultCount; ++i) {
        if (to_lower(kDefaults[i].key) == wanted) {
            return &kDefaults[i];
        }
    }
    return nullptr;
}

}  // namespace ini_defaults

// --------------------------------------------------------------------- parsing

OblivionIni::OblivionIni() = default;

void OblivionIni::clear() {
    entries_.clear();
    by_key_.clear();
    keys_by_section_.clear();
    sections_.clear();
    parsed_lines_ = 0;
    skipped_lines_ = 0;
    last_bad_line_ = 0;
    last_duplicate_line_ = 0;
}

bool OblivionIni::load_from_string(const std::string& text) {
    clear();

    std::string section;
    std::size_t line_number = 0;
    std::size_t pos = 0;
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        pos = 3;
    }

    while (pos < text.size()) {
        const std::size_t newline = text.find_first_of("\r\n", pos);
        std::string line = newline == std::string::npos
                               ? text.substr(pos)
                               : text.substr(pos, newline - pos);
        if (newline == std::string::npos) {
            pos = text.size();
        } else if (text[newline] == '\r' && newline + 1 < text.size() &&
                   text[newline + 1] == '\n') {
            pos = newline + 2;  // CRLF
        } else {
            pos = newline + 1;  // LF or bare CR
        }
        ++line_number;

        const std::string stripped = trim(line);
        if (stripped.empty()) {
            ++skipped_lines_;
            continue;
        }
        if (stripped[0] == ';' || stripped[0] == '#') {
            ++parsed_lines_;
            continue;
        }
        if (stripped[0] == '[') {
            const std::size_t close = stripped.find(']');
            if (close == std::string::npos) {
                ++skipped_lines_;
                last_bad_line_ = line_number;
                continue;
            }
            const std::string rest = trim(stripped.substr(close + 1));
            if (!rest.empty() && rest[0] != ';' && rest[0] != '#') {
                ++skipped_lines_;
                last_bad_line_ = line_number;
                continue;
            }
            section = to_lower(trim(stripped.substr(1, close - 1)).c_str());
            if (!section.empty() &&
                std::find(sections_.begin(), sections_.end(), section) ==
                    sections_.end()) {
                sections_.push_back(section);
            }
            ++parsed_lines_;
            continue;
        }

        const std::size_t equals = stripped.find('=');
        if (equals == std::string::npos) {
            ++skipped_lines_;
            last_bad_line_ = line_number;
            continue;
        }
        const std::string key = to_lower(trim(stripped.substr(0, equals)).c_str());
        if (key.empty()) {
            ++skipped_lines_;
            last_bad_line_ = line_number;
            continue;
        }
        const std::string value = trim(stripped.substr(equals + 1));

        // The engine's registry uses find-or-insert, so the first occurrence of
        // a key wins and later ones are dropped.
        const auto inserted = entries_.emplace(std::make_pair(section, key),
                                              Entry{value, line_number});
        if (!inserted.second) {
            last_duplicate_line_ = line_number;
            ++parsed_lines_;
            continue;
        }
        by_key_.emplace(key, std::make_pair(section, value));
        auto& keys = keys_by_section_[section];
        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
            keys.push_back(key);
        }
        ++parsed_lines_;
    }

    return !entries_.empty();
}

bool OblivionIni::load_file(const std::string& path) {
    clear();
    if (path.empty()) {
        return false;
    }
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return load_from_string(buffer.str());
}

// ------------------------------------------------------------------- resolution

ini_defaults::IniValueType OblivionIni::type_of_file_value(const char* section,
                                                            const char* key,
                                                            const std::string& value) {
    if (const ini_defaults::IniDefaultEntry* exact =
            ini_defaults::find(section, key)) {
        return exact->type;
    }
    if (const ini_defaults::IniDefaultEntry* loose =
            ini_defaults::find_by_key(key)) {
        return loose->type;
    }
    // A key the table does not know: read the type the same way the engine does,
    // with the value shape breaking the tie. Scan codes are hex, not floats, so
    // they have to be checked before the leading letter is trusted.
    const char first = ascii_lower(key[0]);
    if (first != 's' && looks_bare_hex(value)) {
        return ini_defaults::IniValueType::kInteger;
    }
    if (first == 's' || to_lower(key).find("color") != std::string::npos) {
        return ini_defaults::IniValueType::kString;
    }
    if (first == 'f') {
        return ini_defaults::IniValueType::kFloat;
    }
    if (first == 'i' || first == 'b' || first == 'u') {
        return ini_defaults::IniValueType::kInteger;
    }
    long long integer = 0;
    if (parse_int(value.c_str(), integer)) {
        return ini_defaults::IniValueType::kInteger;
    }
    double real = 0.0;
    if (parse_double(value.c_str(), real)) {
        return ini_defaults::IniValueType::kFloat;
    }
    return ini_defaults::IniValueType::kString;
}

OblivionIni::Resolved OblivionIni::resolve(const char* section, const char* key) const {
    Resolved resolved;
    if (key == nullptr || *key == '\0') {
        return resolved;
    }
    const std::string wanted_section = to_lower(section);
    const std::string wanted_key = to_lower(key);

    const auto exact = entries_.find(std::make_pair(wanted_section, wanted_key));
    if (exact != entries_.end()) {
        resolved.text = exact->second.value.c_str();
        resolved.type = type_of_file_value(section, key, exact->second.value);
        resolved.source = ValueSource::kFile;
        return resolved;
    }
    if (const ini_defaults::IniDefaultEntry* entry =
            ini_defaults::find(section, key)) {
        resolved.text = entry->value;
        resolved.type = entry->type;
        resolved.source = ValueSource::kDefault;
        return resolved;
    }
    if (!key_only_fallback_) {
        return resolved;
    }
    const auto loose = by_key_.find(wanted_key);
    if (loose != by_key_.end()) {
        resolved.text = loose->second.second.c_str();
        resolved.type = type_of_file_value(nullptr, key, loose->second.second);
        resolved.source = ValueSource::kFile;
        return resolved;
    }
    if (const ini_defaults::IniDefaultEntry* entry =
            ini_defaults::find_by_key(key)) {
        resolved.text = entry->value;
        resolved.type = entry->type;
        resolved.source = ValueSource::kDefault;
    }
    return resolved;
}

// ---------------------------------------------------------------------- getters

bool OblivionIni::has_file_entry(const char* section, const char* key) const {
    return resolve(section, key).source == ValueSource::kFile;
}

OblivionIni::ValueSource OblivionIni::source_of(const char* section, const char* key) const {
    return resolve(section, key).source;
}

bool OblivionIni::has_value(const char* section, const char* key) const {
    return resolve(section, key).source != ValueSource::kNone;
}

ini_defaults::IniValueType OblivionIni::type_of(const char* section, const char* key) const {
    return resolve(section, key).type;
}

int OblivionIni::get_int(const char* section, const char* key, int fallback) const {
    const Resolved resolved = resolve(section, key);
    if (resolved.text == nullptr) {
        return fallback;
    }
    long long integer = 0;
    if (parse_int(resolved.text, integer)) {
        if (integer < INT_MIN) {
            return INT_MIN;
        }
        if (integer > INT_MAX) {
            return INT_MAX;
        }
        return static_cast<int>(integer);
    }
    double real = 0.0;
    if (parse_double(resolved.text, real)) {
        const double rounded = std::floor(real + 0.5);
        if (rounded < INT_MIN) {
            return INT_MIN;
        }
        if (rounded > INT_MAX) {
            return INT_MAX;
        }
        return static_cast<int>(rounded);
    }
    return fallback;
}

float OblivionIni::get_float(const char* section, const char* key, float fallback) const {
    return static_cast<float>(get_double(section, key, static_cast<double>(fallback)));
}

double OblivionIni::get_double(const char* section, const char* key, double fallback) const {
    const Resolved resolved = resolve(section, key);
    if (resolved.text == nullptr) {
        return fallback;
    }
    double real = 0.0;
    if (parse_double(resolved.text, real)) {
        return real;
    }
    long long integer = 0;
    if (parse_int(resolved.text, integer)) {
        return static_cast<double>(integer);
    }
    return fallback;
}

bool OblivionIni::get_bool(const char* section, const char* key, bool fallback) const {
    const Resolved resolved = resolve(section, key);
    if (resolved.text == nullptr) {
        return fallback;
    }
    const std::string text(resolved.text);
    if (iequals(text, "true") || iequals(text, "yes") || iequals(text, "on")) {
        return true;
    }
    if (iequals(text, "false") || iequals(text, "no") || iequals(text, "off")) {
        return false;
    }
    long long integer = 0;
    if (parse_int(resolved.text, integer)) {
        return integer != 0;
    }
    double real = 0.0;
    if (parse_double(resolved.text, real)) {
        return real != 0.0;
    }
    return fallback;
}

std::string OblivionIni::get_string(const char* section, const char* key,
                                    const std::string& fallback) const {
    const Resolved resolved = resolve(section, key);
    if (resolved.text == nullptr) {
        return fallback;
    }
    return std::string(resolved.text);
}

bool OblivionIni::get_color(const char* section, const char* key, unsigned char& r,
                            unsigned char& g, unsigned char& b) const {
    const Resolved resolved = resolve(section, key);
    if (resolved.text == nullptr) {
        return false;
    }
    const std::string text = trim(resolved.text);
    if (text.empty()) {
        return false;
    }

    unsigned char rgb[3] = {0, 0, 0};
    if (text.find(',') != std::string::npos) {
        if (!parse_color_triplet(text, rgb)) {
            return false;
        }
    } else if (text.size() == 6 && text.find_first_not_of("0123456789abcdefABCDEF") ==
                                       std::string::npos) {
        for (int i = 0; i < 3; ++i) {
            const std::string field = text.substr(static_cast<std::size_t>(i) * 2, 2);
            long long channel = 0;
            if (!parse_int(("0x" + field).c_str(), channel)) {
                return false;
            }
            rgb[i] = clamp_byte(channel);
        }
    } else {
        // The compiled-in registry stores rDebugTextColor as a packed 0xAABBGGRR.
        long long packed = 0;
        if (!parse_int(text.c_str(), packed)) {
            return false;
        }
        rgb[0] = clamp_byte((packed >> 16) & 0xFF);
        rgb[1] = clamp_byte((packed >> 8) & 0xFF);
        rgb[2] = clamp_byte(packed & 0xFF);
    }

    r = rgb[0];
    g = rgb[1];
    b = rgb[2];
    return true;
}

std::size_t OblivionIni::file_entry_count() const {
    return entries_.size();
}

std::size_t OblivionIni::file_section_count() const {
    return sections_.size();
}

const std::vector<std::string>& OblivionIni::sections() const {
    return sections_;
}

const std::vector<std::string>& OblivionIni::keys_of(const std::string& section) const {
    static const std::vector<std::string> kEmpty;
    const auto found = keys_by_section_.find(to_lower(section.c_str()));
    return found == keys_by_section_.end() ? kEmpty : found->second;
}

std::size_t OblivionIni::parsed_line_count() const {
    return parsed_lines_;
}

std::size_t OblivionIni::skipped_line_count() const {
    return skipped_lines_;
}

std::size_t OblivionIni::last_bad_line() const {
    return last_bad_line_;
}

std::size_t OblivionIni::last_duplicate_line() const {
    return last_duplicate_line_;
}

void OblivionIni::set_key_only_fallback(bool enabled) {
    key_only_fallback_ = enabled;
}

bool OblivionIni::key_only_fallback() const {
    return key_only_fallback_;
}

}  // namespace oblivion
