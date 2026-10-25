#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "oblivion_ini_defaults.h"

namespace oblivion {

/**
 * @brief Reader for Oblivion.ini that reproduces the original engine's lookup.
 *
 * The original reads its settings from `<user dir>\Oblivion.ini` and falls back
 * to the compiled-in registry for keys the file does not mention, which is
 * exactly the behaviour a fresh installation relies on (the engine copies
 * Oblivion_default.ini to that path on first run). This class does the same, but
 * resolves the fallback against the merged table in oblivion_ini_defaults.h -
 * the authentic values of both the shipped file and the compiled-in registry.
 *
 * The parser follows the original's own reader:
 *   - `[Section]` opens a section; a `KEY=VALUE` before any section is stored
 *     with an empty section name (three compiled-in keys have no section).
 *   - only whole-line comments are skipped. A `;` or `#` is a comment when it is
 *     the first non-space character of the line; inline comments are part of the
 *     value, exactly as the engine treats them.
 *   - keys and values are trimmed, `=` is split at the first occurrence, values
 *     may legitimately be empty, and lines without `=` are ignored.
 *   - duplicate keys keep the first occurrence, matching the find-or-insert
 *     routine (FUN_004040E0) that registers the compiled-in defaults.
 *   - line endings may be LF, CRLF or CR, with an optional UTF-8 BOM.
 *
 * Section and key lookups are case-insensitive. The class is deliberately free
 * of Android headers so it can be host-tested.
 */
class OblivionIni {
public:
    /// Where the value came from.
    enum class ValueSource : unsigned char {
        kNone = 0,     ///< Neither the file nor the default table has the key.
        kDefault = 1,  ///< Only the authentic default table has the key.
        kFile = 2,     ///< The INI file provides the value (it wins).
    };

    OblivionIni();

    /// Drops every parsed entry. The default table is constant, so it survives.
    void clear();

    /// Parses INI text, discarding the previous contents. Returns true when at
    /// least one key was accepted.
    bool load_from_string(const std::string& text);

    /// Reads and parses a file. Returns false when it cannot be opened or holds
    /// no keys, in which case every lookup falls back to the default table.
    bool load_file(const std::string& path);

    /// True when the INI file itself mentions the key. Covers all three
    /// resolution steps, so a key written into the wrong section still counts.
    bool has_file_entry(const char* section, const char* key) const;

    /// Which layer answers for this key. Use it to log or display where a value
    /// came from; the getters use the same resolution.
    ValueSource source_of(const char* section, const char* key) const;

    /// True when the key is known to either layer.
    bool has_value(const char* section, const char* key) const;

    /// Declared type of the key as reported by whichever layer answers, so a
    /// caller can tell a colour string from a number without parsing. Unknown
    /// keys report kInteger.
    ini_defaults::IniValueType type_of(const char* section, const char* key) const;

    int get_int(const char* section, const char* key, int fallback = 0) const;
    float get_float(const char* section, const char* key, float fallback = 0.0f) const;
    double get_double(const char* section, const char* key, double fallback = 0.0) const;
    bool get_bool(const char* section, const char* key, bool fallback = false) const;

    /// Raw text, trimmed. Strings are never converted, so paths and comma
    /// separated colours come back unchanged.
    std::string get_string(const char* section, const char* key,
                           const std::string& fallback = std::string()) const;

    /// Parses `255,251,233` (the form rDebugTextColor uses), a six digit hex
    /// triplet, or a packed RGB integer. Returns false, leaving the outputs
    /// untouched, when the value is none of those.
    bool get_color(const char* section, const char* key, unsigned char& r,
                   unsigned char& g, unsigned char& b) const;

    /// Number of keys the INI file supplied.
    std::size_t file_entry_count() const;

    /// Number of sections the INI file declared (the empty section that holds
    /// pre-section keys is not counted).
    std::size_t file_section_count() const;

    /// Lowercase section names in file order.
    const std::vector<std::string>& sections() const;

    /// Lowercase key names of one section, in file order.
    const std::vector<std::string>& keys_of(const std::string& section) const;

    /// Number of lines that carried a section header, an assignment, a comment,
    /// or a duplicate assignment.
    std::size_t parsed_line_count() const;

    /// Number of lines that were blank or malformed and therefore ignored.
    std::size_t skipped_line_count() const;

    /// 1-based line number of the last malformed line, or 0 when none.
    std::size_t last_bad_line() const;

    /// 1-based line number of the last duplicate key that was discarded, or 0.
    std::size_t last_duplicate_line() const;

    /// When enabled (the default), a key that neither layer knows exactly is
    /// searched for by key name alone. This reaches the three compiled-in keys
    /// whose name literal omits the section, and it keeps a user's entry usable
    /// when it was written into a different section than the engine expects.
    void set_key_only_fallback(bool enabled);
    bool key_only_fallback() const;

private:
    struct Entry {
        std::string value;
        std::size_t line = 0;
    };

    struct Resolved {
        /// Points either into entries_ (valid until the next clear/load) or into
        /// the constant default table, so it is never null for a found key.
        const char* text = nullptr;
        ini_defaults::IniValueType type = ini_defaults::IniValueType::kInteger;
        ValueSource source = ValueSource::kNone;
    };

    /// file exact -> default exact -> file key-only -> default key-only.
    Resolved resolve(const char* section, const char* key) const;

    /// Type of a value the INI file supplied: the default table wins when it
    /// knows the key, otherwise the key letter and the value shape decide.
    static ini_defaults::IniValueType type_of_file_value(const char* section,
                                                          const char* key,
                                                          const std::string& value);

    std::map<std::pair<std::string, std::string>, Entry> entries_;
    std::map<std::string, std::pair<std::string, std::string>> by_key_;
    std::map<std::string, std::vector<std::string>> keys_by_section_;
    std::vector<std::string> sections_;
    bool key_only_fallback_ = true;
    std::size_t parsed_lines_ = 0;
    std::size_t skipped_lines_ = 0;
    std::size_t last_bad_line_ = 0;
    std::size_t last_duplicate_line_ = 0;
};

}  // namespace oblivion
