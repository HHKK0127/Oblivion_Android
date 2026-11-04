// ============================================================================
// blend_settings_tests.cpp — Host tests for the vanilla BlendSettings table
// See blend_settings_tests.h for scope. Canon: v64 §2 [確定].
// ============================================================================

#include "tests/blend_settings_tests.h"

#include "system/blend_settings.h"

#include <cstring>

namespace oblivion {
namespace blend_settings {

namespace {

unsigned int bits_of(float x) {
    static_assert(sizeof(float) == 4, "IEEE-754 binary32 expected");
    unsigned int b;
    std::memcpy(&b, &x, sizeof(b));
    return b;
}

int count_section(const char* name) {
    int n = 0;
    for (std::size_t i = 0; i < kEntryCount; ++i) {
        if (std::strcmp(kEntries[i].section, name) == 0) {
            ++n;
        }
    }
    return n;
}

int count_type(BlendValueType t) {
    int n = 0;
    for (std::size_t i = 0; i < kEntryCount; ++i) {
        if (kEntries[i].type == t) {
            ++n;
        }
    }
    return n;
}

bool expect_string(const char* section, const char* key, const char* want,
                   std::string& fail_msg) {
    const char* got = nullptr;
    if (!get_string(section, key, &got)) {
        fail_msg = std::string(section) + "/" + key + ": get_string failed";
        return false;
    }
    if (std::strcmp(got, want) != 0) {
        fail_msg = std::string(section) + "/" + key + ": got \"" + got +
                   "\" want \"" + want + "\"";
        return false;
    }
    return true;
}

bool expect_float_bits(const char* section, const char* key,
                       unsigned int want_bits, std::string& fail_msg) {
    float got = 0.0f;
    if (!get_float(section, key, &got)) {
        fail_msg = std::string(section) + "/" + key + ": get_float failed";
        return false;
    }
    if (bits_of(got) != want_bits) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "%s/%s: bits 0x%08X want 0x%08X",
                      section, key, bits_of(got), want_bits);
        fail_msg = buf;
        return false;
    }
    return true;
}

}  // namespace

bool BlendSettingsTests::runAllTests() {
    results.clear();
    test_entry_count();
    test_section_composition();
    test_type_composition();
    test_float_bits_all();
    test_string_values_spot_check();
    test_typo_entry_verbatim();
    test_bone_name_entries();
    test_case_insensitive_find();
    test_getters_type_mismatch();
    test_parse_pair_accept();
    test_parse_pair_reject();

    for (const auto& r : results) {
        if (!r.passed) {
            return false;
        }
    }
    return true;
}

void BlendSettingsTests::test_entry_count() {
    std::string msg;
    if (kEntryCount != 62) {
        msg = "kEntryCount is " + std::to_string(kEntryCount) + ", want 62";
    }
    record("entry_count_62", kEntryCount == 62, msg);
}

void BlendSettingsTests::test_section_composition() {
    std::string msg;
    const bool ok = count_section("HIT") == 20 &&
                    count_section("QUADHIT") == 14 &&
                    count_section("DEFAULT") == 28;
    if (!ok) {
        msg = "HIT=" + std::to_string(count_section("HIT")) + " (want 20), " +
              "QUADHIT=" + std::to_string(count_section("QUADHIT")) +
              " (want 14), " +
              "DEFAULT=" + std::to_string(count_section("DEFAULT")) +
              " (want 28)";
    }
    record("section_composition", ok, msg);
}

void BlendSettingsTests::test_type_composition() {
    std::string msg;
    const int nf = count_type(BlendValueType::kFloat);
    const int ns = count_type(BlendValueType::kString);
    const bool ok = (nf == 14 && ns == 48);
    if (!ok) {
        msg = "float=" + std::to_string(nf) + " (want 14), string=" +
              std::to_string(ns) + " (want 48)";
    }
    record("type_composition", ok, msg);
}

// All 14 float entries checked bit-exactly (v64 §2.3 hex-verified patterns).
void BlendSettingsTests::test_float_bits_all() {
    struct Expect {
        const char* section;
        const char* key;
        unsigned int bits;
    };
    static const Expect kExpect[] = {
        {"HIT", "fGetUpTime", 0x3F800000u},      // 1.0
        {"HIT", "fKnockDownTime", 0x3E800000u},  // 0.25
        {"HIT", "fMinVelGain", 0x3F733333u},     // 0.95
        {"HIT", "fMinHeirGain", 0x3E99999Au},    // 0.3
        {"HIT", "fHFMin", 0x00000000u},          // 0.0
        {"HIT", "fHFMax", 0x00000000u},          // 0.0
        {"HIT", "fBFMin", 0x00000000u},          // 0.0
        {"HIT", "fBFMax", 0x00000000u},          // 0.0
        {"DEFAULT", "fHighTrans", 0x3D8F5C29u},  // 0.07
        {"DEFAULT", "fHighRot", 0x3BA3D70Au},    // 0.005
        {"DEFAULT", "fLowTrans", 0x3F333333u},   // 0.7
        {"DEFAULT", "fLowRot", 0x3E4CCCCDu},     // 0.2
        {"DEFAULT", "fPassOutTime", 0x3F99999Au},// 1.2
        {"DEFAULT", "fPassOutForce", 0xC1200000u}// -10.0
    };
    std::string msg;
    bool ok = true;
    for (const auto& e : kExpect) {
        if (!expect_float_bits(e.section, e.key, e.bits, msg)) {
            ok = false;
            break;
        }
    }
    record("float_bits_all_14", ok, msg);
}

void BlendSettingsTests::test_string_values_spot_check() {
    std::string msg;
    bool ok = true;
    ok = expect_string("HIT", "sRHand", "1.0, 1.0", msg) && ok;
    if (!ok) {
        record("string_values_spot_check", false, msg);
        return;
    }
    ok = expect_string("HIT", "sRUpperArm", "0.2, 0.5", msg) && ok;
    if (!ok) {
        record("string_values_spot_check", false, msg);
        return;
    }
    ok = expect_string("HIT", "sSpine2", "0.5, 0.7", msg) && ok;
    if (!ok) {
        record("string_values_spot_check", false, msg);
        return;
    }
    ok = expect_string("QUADHIT", "sHead", "0.3, 0.9", msg) && ok;
    if (!ok) {
        record("string_values_spot_check", false, msg);
        return;
    }
    ok = expect_string("DEFAULT", "sHead", "1.0, 1.0", msg) && ok;
    if (!ok) {
        record("string_values_spot_check", false, msg);
        return;
    }
    ok = expect_string("DEFAULT", "sPonyTail", "1.0, 1.0", msg);
    record("string_values_spot_check", ok, msg);
}

// The vanilla typo at DEFAULT/sRHand must be preserved byte-for-byte.
void BlendSettingsTests::test_typo_entry_verbatim() {
    std::string msg;
    const bool ok = expect_string("DEFAULT", "sRHand", "1.0f, 1.0", msg);
    record("typo_entry_verbatim", ok, msg);
}

void BlendSettingsTests::test_bone_name_entries() {
    std::string msg;
    bool ok = expect_string("HIT", "sHitSpot", "Bip01 Spine2", msg);
    ok = expect_string("HIT", "sHitStart", "Bip01 Spine1", msg) && ok;
    ok = expect_string("QUADHIT", "sHitSpot", "Bip01 Head", msg) && ok;
    ok = expect_string("QUADHIT", "sHitStart", "Bip01 Head", msg) && ok;
    record("bone_name_entries", ok, msg);
}

void BlendSettingsTests::test_case_insensitive_find() {
    std::string msg;
    bool ok = true;
    const BlendSettingEntry* e = find("hit", "fgetuptime");
    if (!e || e->float_bits != 0x3F800000u) {
        ok = false;
        msg = "find(\"hit\", \"fgetuptime\") failed";
    }
    if (ok) {
        e = find("QuadHit", "SHITSPOT");
        if (!e || std::strcmp(e->string_value, "Bip01 Head") != 0) {
            ok = false;
            msg = "find(\"QuadHit\", \"SHITSPOT\") failed";
        }
    }
    if (ok) {
        float v = 0.0f;
        if (!get_float("DeFaUlT", "FPASSOUTFORCE", &v) ||
            bits_of(v) != 0xC1200000u) {
            ok = false;
            msg = "case-insensitive get_float failed";
        }
    }
    if (ok && find("HIT", "sNoSuchKey") != nullptr) {
        ok = false;
        msg = "absent key returned non-null";
    }
    record("case_insensitive_find", ok, msg);
}

void BlendSettingsTests::test_getters_type_mismatch() {
    std::string msg;
    bool ok = true;
    float fv = 0.0f;
    if (get_float("HIT", "sRHand", &fv)) {
        ok = false;
        msg = "get_float on string entry should fail";
    }
    if (ok) {
        const char* sv = nullptr;
        if (get_string("HIT", "fGetUpTime", &sv)) {
            ok = false;
            msg = "get_string on float entry should fail";
        }
    }
    if (ok && (get_float("HIT", "fGetUpTime", nullptr) ||
               get_string("HIT", "sRHand", nullptr))) {
        ok = false;
        msg = "null out pointer should fail";
    }
    if (ok && (find(nullptr, "x") || find("HIT", nullptr))) {
        ok = false;
        msg = "null section/key should fail";
    }
    record("getters_type_mismatch", ok, msg);
}

void BlendSettingsTests::test_parse_pair_accept() {
    struct Case {
        const char* text;
        float lo;
        float hi;
    };
    static const Case kCases[] = {
        {"1.0, 1.0", 1.0f, 1.0f},
        {"0.2, 0.5", 0.2f, 0.5f},
        {"0.3,0.9", 0.3f, 0.9f},           // no space after comma
        {"  0.5, 0.7  ", 0.5f, 0.7f},      // surrounding spaces
        {"\t1.0, 2.0\t", 1.0f, 2.0f},      // tabs
        {"-10.0, 1.0", -10.0f, 1.0f},      // negative
        {"0.0, 0.0", 0.0f, 0.0f},
    };
    std::string msg;
    bool ok = true;
    for (const auto& c : kCases) {
        float lo = 999.0f, hi = 999.0f;
        if (!parse_blend_pair(c.text, &lo, &hi)) {
            ok = false;
            msg = std::string("accept failed: \"") + c.text + "\"";
            break;
        }
        if (bits_of(lo) != bits_of(c.lo) || bits_of(hi) != bits_of(c.hi)) {
            ok = false;
            msg = std::string("wrong value for: \"") + c.text + "\"";
            break;
        }
    }
    record("parse_pair_accept", ok, msg);
}

void BlendSettingsTests::test_parse_pair_reject() {
    static const char* const kRejects[] = {
        "1.0f, 1.0",      // vanilla typo — deterministically rejected
        "1.0F, 1.0",      // uppercase suffix variant
        "1.0, 1.0f",      // suffix on second number
        "1.0 1.0",        // missing comma
        "1.0, 1.0, 2.0",  // trailing garbage
        "abc, 1.0",       // non-numeric
        "",               // empty
        ", 1.0",          // missing first number
        "1.0,",           // missing second number
        "1.0, x",         // non-numeric second
    };
    std::string msg;
    bool ok = true;
    for (const char* text : kRejects) {
        float lo = 0.0f, hi = 0.0f;
        if (parse_blend_pair(text, &lo, &hi)) {
            ok = false;
            msg = std::string("should reject: \"") + text + "\"";
            break;
        }
    }
    if (ok) {
        float lo = 0.0f, hi = 0.0f;
        if (parse_blend_pair(nullptr, &lo, &hi) ||
            parse_blend_pair("1.0, 1.0", nullptr, &hi) ||
            parse_blend_pair("1.0, 1.0", &lo, nullptr)) {
            ok = false;
            msg = "null args should reject";
        }
    }
    record("parse_pair_reject", ok, msg);
}

void BlendSettingsTests::record(const std::string& name, bool passed,
                                const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int BlendSettingsTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (r.passed) {
            ++n;
        }
    }
    return n;
}

int BlendSettingsTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (!r.passed) {
            ++n;
        }
    }
    return n;
}

std::string BlendSettingsTests::getSummary() const {
    return std::to_string(getPassCount()) + "/" +
           std::to_string(results.size()) + " passed";
}

}  // namespace blend_settings
}  // namespace oblivion
