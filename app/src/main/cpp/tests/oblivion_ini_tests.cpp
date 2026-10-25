// Oblivion.ini reader tests - see oblivion_ini_tests.h for the rationale.

#include "oblivion_ini_tests.h"

#include "../system/oblivion_ini.h"

#include <chrono>
#include <cmath>
#include <string>

using oblivion::OblivionIni;
namespace defaults = oblivion::ini_defaults;

namespace {

/// Lowercase (section, key) of a table entry, i.e. the table's own sort key.
bool entry_less(const defaults::IniDefaultEntry& a,
                const defaults::IniDefaultEntry& b) {
    std::string as = a.section;
    std::string bs = b.section;
    std::string ak = a.key;
    std::string bk = b.key;
    for (char& c : as) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char& c : bs) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char& c : ak) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char& c : bk) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (as != bs) return as < bs;
    return ak < bk;
}

/// The shipped Oblivion_default.ini is CRLF; the engine also tolerates LF and CR.
const std::string kSample =
    "; comment\r\n"
    "\r\n"
    "[General]\r\n"
    "sStartingCell=\r\n"
    "sStartingCell=NotThisOne\r\n"
    "[General] ; trailing comment\r\n"
    "# hash comment\r\n"
    "no equals here\r\n"
    "[unclosed\r\n"
    "=novalue\r\n"
    "[Controls]\r\n"
    "Forward=0011FFFF\r\n"
    "fJoystickLookLRMult=0.0020\r\n";

}  // namespace

void OblivionIniTests::record(const std::string& name, bool passed,
                              const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int OblivionIniTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results)
        if (r.passed) n++;
    return n;
}

int OblivionIniTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results)
        if (!r.passed) n++;
    return n;
}

std::string OblivionIniTests::getSummary() const {
    return "Oblivion INI Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool OblivionIniTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    const auto check = [&](const std::string& name, bool ok,
                           const std::string& msg = std::string()) {
        record(name, ok, msg);
    };
    const auto check_int = [&](const std::string& name, long long got, long long want) {
        record(name, got == want,
               "got " + std::to_string(got) + ", want " + std::to_string(want));
    };
    const auto check_str = [&](const std::string& name, const std::string& got,
                               const std::string& want) {
        record(name, got == want, "got \"" + got + "\", want \"" + want + "\"");
    };
    const auto check_real = [&](const std::string& name, double got, double want) {
        record(name, std::fabs(got - want) < 1e-6,
               "got " + std::to_string(got) + ", want " + std::to_string(want));
    };

    // ------------------------------------------------------------------
    // 1. The merged default table: size and composition.
    // ------------------------------------------------------------------
    {
        check_int("default table has 657 entries", defaults::kDefaultCount, 657);

        int integers = 0;
        int floats = 0;
        int strings = 0;
        int from_file = 0;
        int from_engine = 0;
        int from_both = 0;
        int empty_key = 0;
        int null_field = 0;
        for (std::size_t i = 0; i < defaults::kDefaultCount; ++i) {
            const defaults::IniDefaultEntry& e = defaults::kDefaults[i];
            if (e.section == nullptr || e.key == nullptr || e.value == nullptr) {
                ++null_field;
                continue;
            }
            switch (e.type) {
                case defaults::IniValueType::kInteger: ++integers; break;
                case defaults::IniValueType::kFloat: ++floats; break;
                case defaults::IniValueType::kString: ++strings; break;
            }
            switch (e.provenance) {
                case defaults::kFromFile: ++from_file; break;
                case defaults::kFromEngine: ++from_engine; break;
                case defaults::kFromBoth: ++from_both; break;
                default: break;
            }
            if (*e.key == '\0') ++empty_key;
        }
        check_int("342 Integer entries", integers, 342);
        check_int("253 Float entries", floats, 253);
        check_int("62 String entries", strings, 62);
        check_int("404 entries from both sources", from_both, 404);
        check_int("85 entries only in the shipped file", from_file, 85);
        check_int("168 entries only in the compiled-in registry", from_engine, 168);
        check_int("no entry has a null field", null_field, 0);
        check_int("no entry has an empty key", empty_key, 0);
    }

    // ------------------------------------------------------------------
    // 2. Ordering. `find` is a binary search, so the table must be sorted by
    //    (lowercase section, lowercase key) and must not repeat a pair.
    // ------------------------------------------------------------------
    {
        int out_of_order = 0;
        std::size_t first_bad = 0;
        for (std::size_t i = 1; i < defaults::kDefaultCount; ++i) {
            if (entry_less(defaults::kDefaults[i], defaults::kDefaults[i - 1])) {
                if (out_of_order == 0) first_bad = i;
                ++out_of_order;
            }
        }
        check_int("table is sorted by lowercase section then key", out_of_order, 0);
        if (out_of_order != 0) {
            record("first out-of-order entry",
                   false,
                   std::string("index ") + std::to_string(first_bad) + ": [" +
                       defaults::kDefaults[first_bad].section + "] " +
                       defaults::kDefaults[first_bad].key);
        }

        int duplicates = 0;
        for (std::size_t i = 1; i < defaults::kDefaultCount; ++i) {
            if (!entry_less(defaults::kDefaults[i - 1], defaults::kDefaults[i]) &&
                !entry_less(defaults::kDefaults[i], defaults::kDefaults[i - 1])) {
                ++duplicates;
            }
        }
        check_int("no duplicate (section, key)", duplicates, 0);
    }

    // Every entry must be reachable through the binary search. This is the test
    // that would have caught the [HAVOK] / [Havok] ordering break, where one row
    // sat at the wrong end of its section.
    {
        int unreachable = 0;
        std::string first_unreachable;
        for (std::size_t i = 0; i < defaults::kDefaultCount; ++i) {
            const defaults::IniDefaultEntry& e = defaults::kDefaults[i];
            if (defaults::find(e.section, e.key) != &e) {
                if (unreachable == 0) {
                    first_unreachable = std::string("[") + e.section + "] " + e.key;
                }
                ++unreachable;
            }
        }
        check_int("every entry is found by find()", unreachable, 0);
        if (unreachable != 0) {
            record("first entry find() misses", false, first_unreachable);
        }
    }

    // ------------------------------------------------------------------
    // 3. Lookup behaviour: case-insensitive, both spellings of the sections the
    //    two sources disagree on, and nullptr for the unknown.
    // ------------------------------------------------------------------
    {
        const defaults::IniDefaultEntry* forward =
            defaults::find("Controls", "Forward");
        check("find([Controls] Forward)", forward != nullptr);
        if (forward != nullptr) {
            check_str("[Controls] Forward is the scan code", forward->value, "0011FFFF");
            check("[Controls] Forward is an Integer",
                  forward->type == defaults::IniValueType::kInteger);
        }

        const defaults::IniDefaultEntry* archive =
            defaults::find("Archive", "sArchiveList");
        check("find([Archive] sArchiveList)", archive != nullptr);
        if (archive != nullptr) {
            check("the BSA list is not truncated",
                  std::string(archive->value).find("Oblivion - Voices2.bsa") !=
                      std::string::npos);
        }

        check("find() ignores the case of the section",
              defaults::find("archive", "sArchiveList") != nullptr);
        check("find() ignores the case of the key",
              defaults::find("ARCHIVE", "SARCHIVELIST") != nullptr);

        // The regression: the shipped file writes [HAVOK], the registry [Havok].
        check("find([HAVOK] ...) reaches a registry-spelled entry",
              defaults::find("HAVOK", "iHavokSkipFrameCountTEST") != nullptr);
        check("find([Havok] ...) reaches a file-spelled entry",
              defaults::find("Havok", "iUpdateType") != nullptr);
        check("find([PathFinding] ...) reaches a registry-spelled entry",
              defaults::find("PathFinding", "fPathMinCarriedItemDistance") != nullptr ||
                  defaults::find("PathFinding", "iPathMaxCarriedItems") != nullptr);

        check("find([Archive] sArchiveList) is String",
              archive != nullptr && archive->type == defaults::IniValueType::kString);
        check("find([Audio] fDefaultMasterVolume) is Float",
              defaults::find("Audio", "fDefaultMasterVolume") != nullptr &&
                  defaults::find("Audio", "fDefaultMasterVolume")->type ==
                      defaults::IniValueType::kFloat);
        check("find([General] iNumHavokThreads) is unknown to the table",
              defaults::find("General", "iNumHavokThreads") == nullptr);
        check("find() returns null for an unknown key",
              defaults::find("Display", "noSuchKeyHere") == nullptr);
        check("find() returns null for a null key",
              defaults::find("Display", nullptr) == nullptr);
        check("find() returns null for an empty key",
              defaults::find("Display", "") == nullptr);
    }

    // ------------------------------------------------------------------
    // 4. The three compiled-in keys whose name literal carries no section.
    // ------------------------------------------------------------------
    {
        const defaults::IniDefaultEntry* hdr =
            defaults::find_by_key("iLastHDRSetting");
        check("find_by_key() reaches the section-less iLastHDRSetting", hdr != nullptr);
        if (hdr != nullptr) {
            check_str("iLastHDRSetting has an empty section", hdr->section, "");
        }
        check("find_by_key() is case-insensitive",
              defaults::find_by_key("ILASTHDRSETTING") != nullptr);
        check("find_by_key() reaches iDetectionHighNumPicks",
              defaults::find_by_key("iDetectionHighNumPicks") != nullptr);
        check("find_by_key() reaches fLowPerfCombatantVoiceDistance",
              defaults::find_by_key("fLowPerfCombatantVoiceDistance") != nullptr);
        check("find_by_key() returns null when the key is unknown",
              defaults::find_by_key("noSuchKeyHere") == nullptr);
    }

    // ------------------------------------------------------------------
    // 5. Parsing: line endings, comments, malformed lines, duplicates.
    // ------------------------------------------------------------------
    {
        OblivionIni ini;
        check("load_from_string accepts the sample", ini.load_from_string(kSample));
        check_int("sample accepted 3 keys", ini.file_entry_count(), 3);
        check_int("sample declared 2 sections", ini.file_section_count(), 2);
        check_int("sample parsed 9 lines", ini.parsed_line_count(), 9);
        check_int("sample skipped 4 lines", ini.skipped_line_count(), 4);
        check_int("last malformed line is 10", ini.last_bad_line(), 10);
        check_int("last duplicate line is 5", ini.last_duplicate_line(), 5);

        const std::vector<std::string> expected_sections = {"general", "controls"};
        check("sections are lowercase and in file order",
              ini.sections() == expected_sections);

        const std::vector<std::string> expected_keys = {"forward",
                                                        "fjoysticklooklrmult"};
        check("keys_of() is lowercase and in file order",
              ini.keys_of("Controls") == expected_keys);
        check("keys_of() ignores the case of the section",
              ini.keys_of("CONTROLS") == expected_keys);
        check("keys_of() of an unknown section is empty",
              ini.keys_of("nope").empty());

        // The duplicate kept the first occurrence.
        check_str("the first duplicate wins", ini.get_string("General", "sStartingCell"),
                  "");
        check("a file value is reported as kFile",
              ini.source_of("General", "sStartingCell") ==
                  OblivionIni::ValueSource::kFile);
        check("has_file_entry() sees file keys", ini.has_file_entry("general", "forward"));

        // A line without '=' or with an empty key is malformed; so is [unclosed.
        check("a malformed line does not become a key",
              !ini.has_value("General", "no equals here"));
        check("a section header with junk after ']' is malformed",
              !ini.has_value("unclosed", "anything"));
        check("an empty key is not stored", !ini.has_file_entry("Controls", ""));
    }

    // CR-only endings and a UTF-8 BOM.
    {
        OblivionIni ini;
        check("load_from_string accepts CR-only text",
              ini.load_from_string("a=1\rb=2\r"));
        check_int("CR-only text parsed 2 keys", ini.file_entry_count(), 2);
        check_str("CR-only first key", ini.get_string("", "a"), "1");
        check_int("CR-only keys land in the empty section", ini.file_section_count(), 0);
    }
    {
        OblivionIni ini;
        check("load_from_string skips the UTF-8 BOM",
              ini.load_from_string("\xEF\xBB\xBF" "[Menu]\r\nsColor=00FF80\r\n"));
        check_str("the BOM does not corrupt the first section",
                  ini.keys_of("Menu").empty() ? std::string("<missing>")
                                              : ini.keys_of("Menu")[0],
                  "scolor");
    }

    // Only whole-line comments are comments; the engine keeps an inline one as
    // part of the value.
    {
        OblivionIni ini;
        ini.load_from_string("[Misc]\nsFoo=1 ; not a comment\n");
        check_str("an inline comment stays in the value",
                  ini.get_string("Misc", "sFoo"), "1 ; not a comment");
    }
    {
        OblivionIni ini;
        ini.load_from_string(";only a comment\n#also a comment\n");
        check_int("a comment-only file yields no keys", ini.file_entry_count(), 0);
        check_int("comments still count as parsed lines", ini.parsed_line_count(), 2);
        check("load_from_string reports failure without keys",
              !ini.load_from_string(";only a comment\n"));
    }

    // Whitespace is trimmed from the key and from both ends of the value.
    {
        OblivionIni ini;
        ini.load_from_string("[  Audio  ]\r\n  fDefaultMasterVolume =   1.0000   \r\n");
        check("surrounding whitespace is trimmed",
              ini.get_string("Audio", "fDefaultMasterVolume") == "1.0000");
        check_int("a padded section header registers once", ini.file_section_count(), 1);
    }

    // ------------------------------------------------------------------
    // 6. Resolution order: file, then the default table, then key-only.
    // ------------------------------------------------------------------
    {
        OblivionIni ini;
        check("the default table answers with no file loaded",
              ini.source_of("Display", "iSize W") == OblivionIni::ValueSource::kDefault);
        check_int("the default resolution is the shipped value",
                  ini.get_int("Display", "iSize W"), 640);
        check_real("a default Float converts to double",
                   ini.get_double("Audio", "fDefaultMasterVolume"), 1.0);
        check("an unknown key has no value", !ini.has_value("Display", "nope"));
    }
    {
        OblivionIni ini;
        ini.load_from_string("[Controls]\nfVersion=1.6\n");
        check("the file wins over the default table",
              ini.source_of("Controls", "fVersion") == OblivionIni::ValueSource::kFile);
        check_real("the file value is used", ini.get_double("Controls", "fVersion"), 1.6);
        check_real("the untouched sibling still comes from the table",
                   ini.get_double("Audio", "fDefaultMasterVolume"), 1.0);
    }

    // Real values from the shipped Oblivion_default.ini.
    {
        OblivionIni ini;
        ini.load_from_string(
            "[Audio]\r\n"
            "fDefaultMasterVolume=1.0000\r\n"
            "fDefaultVoiceVolume=0.8000\r\n"
            "fDefaultFootVolume=0.8000\r\n"
            "fDefaultMusicVolume=0.3000\r\n"
            "[GamePlay]\r\n"
            "bGeneralSubtitles=0\r\n");
        check_real("shipped master volume", ini.get_double("Audio", "fDefaultMasterVolume"),
                   1.0);
        check_real("shipped voice volume", ini.get_float("Audio", "fDefaultVoiceVolume"),
                   0.8f);
        check_real("shipped foot volume", ini.get_float("Audio", "fDefaultFootVolume"), 0.8f);
        check_real("shipped music volume", ini.get_float("Audio", "fDefaultMusicVolume"),
                   0.3f);
        check("shipped bGeneralSubtitles is off", !ini.get_bool("GamePlay", "bGeneralSubtitles"));
    }

    // Key-only fallback: a user's key written into the wrong section, and the
    // compiled-in keys that have no section at all.
    {
        OblivionIni ini;
        ini.load_from_string("[General]\niSize W=1024\n");
        check("a misplaced file key is still found",
              ini.source_of("Display", "iSize W") == OblivionIni::ValueSource::kFile);
        check_int("the misplaced key keeps its file value",
                  ini.get_int("Display", "iSize W"), 1024);
        check("has_file_entry() covers the key-only step",
              ini.has_file_entry("Display", "iSize W"));

        ini.set_key_only_fallback(false);
        check("with the fallback off the file value is not used",
              ini.source_of("Display", "iSize W") == OblivionIni::ValueSource::kDefault);
        check_int("with the fallback off the table value answers",
                  ini.get_int("Display", "iSize W"), 640);
        ini.set_key_only_fallback(true);

        check("a section-less default is reachable by key alone",
              ini.get_int("Display", "iLastHDRSetting", -999) == -1);
        ini.set_key_only_fallback(false);
        check("a section-less default is unreachable without the fallback",
              !ini.has_value("Display", "iLastHDRSetting"));
        check_int("the fallback flag round-trips",
                  ini.key_only_fallback() ? 1 : 0, 0);
    }

    // ------------------------------------------------------------------
    // 7. Getters.
    // ------------------------------------------------------------------
    {
        OblivionIni ini;
        // [Controls] writes DirectInput scan codes as bare hex, no 0x prefix.
        check_int("a bare hex scan code is hex, not decimal",
                  ini.get_int("Controls", "Forward"), 1179647);
        check_int("an explicit 0x prefix is hex",
                  ini.get_int("Controls", "Forward", 0), 0x0011FFFF);
        // A pure decimal string must not be mistaken for hex.
        check_int("a pure decimal stays decimal", ini.get_int("Display", "iSize W"), 640);

        ini.load_from_string(
            "[Misc]\n"
            "fRounded=2.5\n"
            "fDown=2.4\n"
            "fNegative=-2.5\n"
            "iBig=4294967295\n"
            "iHuge=99999999999\n"
            "sText=hello\n"
            "sEmpty=\n");
        check_int("a float rounds half up", ini.get_int("Misc", "fRounded"), 3);
        check_int("a float below the half rounds down", ini.get_int("Misc", "fDown"), 2);
        check_int("a negative float rounds away from zero",
                  ini.get_int("Misc", "fNegative"), -2);
        check_int("a 32-bit unsigned value saturates to INT_MAX",
                  ini.get_int("Misc", "iBig"), 2147483647);
        check_int("an out-of-range value falls back through the double path",
                  ini.get_int("Misc", "iHuge"), 2147483647);
        check_int("a non-numeric value returns the fallback",
                  ini.get_int("Misc", "sText", -7), -7);
        check_int("an empty value returns the fallback",
                  ini.get_int("Misc", "sEmpty", -8), -8);
        check_real("get_double reads an integer value",
                   ini.get_double("Misc", "iBig"), 4294967295.0);
        check_real("get_double reads a float value", ini.get_double("Misc", "fRounded"), 2.5);
        check_str("get_string returns the raw text", ini.get_string("Misc", "sText"),
                  "hello");
    }
    {
        OblivionIni ini;
        ini.load_from_string(
            "[Misc]\n"
            "bTrue=true\n"
            "bYes=YES\n"
            "bOn=on\n"
            "bFalse=false\n"
            "bNo=No\n"
            "bOff=off\n"
            "bOne=1\n"
            "bZero=0\n"
            "bNeg=-1\n"
            "bHalf=0.5\n"
            "bWord=maybe\n");
        check("true is true", ini.get_bool("Misc", "bTrue"));
        check("yes is true", ini.get_bool("Misc", "bYes"));
        check("on is true", ini.get_bool("Misc", "bOn"));
        check("false is false", !ini.get_bool("Misc", "bFalse", true));
        check("no is false", !ini.get_bool("Misc", "bNo", true));
        check("off is false", !ini.get_bool("Misc", "bOff", true));
        check("a non-zero integer is true", ini.get_bool("Misc", "bOne"));
        check("zero is false", !ini.get_bool("Misc", "bZero", true));
        check("a negative integer is true", ini.get_bool("Misc", "bNeg"));
        check("a non-zero float is true", ini.get_bool("Misc", "bHalf"));
        check("an unparsable word returns the fallback",
              !ini.get_bool("Misc", "bWord", false));
        check("an unknown key returns the fallback", !ini.get_bool("Misc", "bMissing", false));
    }

    // ------------------------------------------------------------------
    // 8. Declared types and colours.
    // ------------------------------------------------------------------
    {
        OblivionIni ini;
        check("the table declares iSize W as Integer",
              ini.type_of("Display", "iSize W") == defaults::IniValueType::kInteger);
        check("the table declares fDefaultMasterVolume as Float",
              ini.type_of("Audio", "fDefaultMasterVolume") ==
                  defaults::IniValueType::kFloat);
        check("the table declares an empty string key as String",
              ini.type_of("General", "sStartingCell") == defaults::IniValueType::kString);
        check("an unknown key reports Integer",
              ini.type_of("Display", "nope") == defaults::IniValueType::kInteger);

        // The table wins for a key it knows, whatever the file wrote; for an
        // unknown key the value shape decides, with bare hex beating the letter.
        ini.load_from_string(
            "[Misc]\n"
            "fWeird=0011FFFF\n"
            "sHex=0011FFFF\n"
            "uScan=0011FFFF\n"
            "sName=bar\n"
            "fReal=1.5\n"
            "iCount=7\n"
            "xOther=xyz\n");
        check("bare hex wins over the leading letter",
              ini.type_of("Misc", "fWeird") == defaults::IniValueType::kInteger);
        check("an 's' key is a String even with a hex-looking value",
              ini.type_of("Misc", "sHex") == defaults::IniValueType::kString);
        check("a 'u' key with a hex value is an Integer",
              ini.type_of("Misc", "uScan") == defaults::IniValueType::kInteger);
        check("an 's' key is a String",
              ini.type_of("Misc", "sName") == defaults::IniValueType::kString);
        check("an 'f' key with a real value is a Float",
              ini.type_of("Misc", "fReal") == defaults::IniValueType::kFloat);
        check("an 'i' key with an integer value is an Integer",
              ini.type_of("Misc", "iCount") == defaults::IniValueType::kInteger);
        check("an unknown key letter with a word value is a String",
              ini.type_of("Misc", "xOther") == defaults::IniValueType::kString);
        check("a key this table knows keeps the table's type",
              ini.type_of("Display", "iSize W") == defaults::IniValueType::kInteger);
    }
    {
        OblivionIni ini;
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;

        // The shipped file writes the debug text colour as a triplet.
        check("the triplet colour parses", ini.get_color("Menu", "rDebugTextColor", r, g, b));
        check_int("triplet red", r, 255);
        check_int("triplet green", g, 251);
        check_int("triplet blue", b, 233);

        ini.load_from_string(
            "[Menu]\n"
            "sHexColor=00FF80\n"
            "nPacked=16711680\n"
            "sBroken=hello\n"
            "sShort=255,251\n"
            "sLong=1,2,3,4\n"
            "sClamped=300,0,0\n");
        r = g = b = 0;
        check("a six digit hex colour parses", ini.get_color("Menu", "sHexColor", r, g, b));
        check_int("hex red", r, 0);
        check_int("hex green", g, 255);
        check_int("hex blue", b, 128);

        r = g = b = 0;
        check("a packed integer colour parses", ini.get_color("Menu", "nPacked", r, g, b));
        check_int("packed red", r, 255);
        check_int("packed green", g, 0);
        check_int("packed blue", b, 0);

        r = 9; g = 9; b = 9;
        check("a non-colour value is rejected", !ini.get_color("Menu", "sBroken", r, g, b));
        check("a rejected colour leaves the outputs untouched", r == 9 && g == 9 && b == 9);
        check("a two-field triplet is rejected", !ini.get_color("Menu", "sShort", r, g, b));
        check("a four-field triplet is rejected", !ini.get_color("Menu", "sLong", r, g, b));
        check("an unknown key is rejected", !ini.get_color("Menu", "sMissing", r, g, b));

        r = g = b = 0;
        check("a channel above 255 clamps", ini.get_color("Menu", "sClamped", r, g, b));
        check_int("clamped red", r, 255);
    }
    {
        // Empty string values are real values, not missing keys: sStartingCell
        // and the sTestFileN keys all point at one empty literal in the binary.
        OblivionIni ini;
        check("an empty default is still a value",
              ini.has_value("General", "sStartingCell"));
        check_str("an empty default reads as an empty string",
                  ini.get_string("General", "sStartingCell"), "");
        check("an empty string falls back for get_int",
              ini.get_int("General", "sStartingCell", -1) == -1);
        check("an empty string falls back for get_bool",
              !ini.get_bool("General", "sStartingCell", false));
    }

    // ------------------------------------------------------------------
    // 9. File loading and reset.
    // ------------------------------------------------------------------
    {
        OblivionIni ini;
        ini.load_from_string("[Display]\niSize W=800\n");
        check_int("the loaded value is used", ini.get_int("Display", "iSize W"), 800);
        ini.clear();
        check_int("clear() drops the file keys", ini.file_entry_count(), 0);
        check_int("clear() keeps the default fallback", ini.get_int("Display", "iSize W"),
                  640);
        check("load_file() on a missing path fails",
              !ini.load_file("no/such/oblivion.ini"));
        check("load_file() on an empty path fails", !ini.load_file(""));
        check_int("a failed load leaves the defaults usable",
                  ini.get_int("Display", "iSize W"), 640);
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    for (auto& r : results)
        r.durationMs = totalMs / static_cast<float>(results.size());

    return getFailCount() == 0;
}
