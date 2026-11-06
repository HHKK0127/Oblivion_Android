// ============================================================================
// blend_settings.cpp — Vanilla ragdoll "BlendSettings" default table (62 entries)
//
// RE provenance: tmp/re/v64_section7_remaining.md §2 [確定].
//   Entry array @0xB11A24 (stride 8), CRT$XCU init thunks @0xA29084,
//   singleton @0xB11C40 vtable 0xA566A4, AddSetting @0x4040E0.
// All float patterns are hex-verified against the shipped image. Strings
// follow the vanilla values except sRHand:DEFAULT, whose shipped typo is
// normalized to the strict pair format used by the Android parser.
// ============================================================================

#include "blend_settings.h"

#include <cstring>
#include <cstdlib>
#include <cerrno>

namespace oblivion {
namespace blend_settings {

namespace {

constexpr unsigned int kZeroBits = 0x00000000u;     // 0.0f

inline constexpr BlendSettingEntry f(const char* section, const char* key,
                                     unsigned int bits) {
    return {section, key, BlendValueType::kFloat, bits, nullptr};
}

inline constexpr BlendSettingEntry s(const char* section, const char* key,
                                     const char* value) {
    return {section, key, BlendValueType::kString, 0u, value};
}

inline char ascii_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// ASCII case-insensitive equality (vanilla INI-style registry semantics).
bool iequals(const char* a, const char* b) {
    while (*a && *b) {
        if (ascii_lower(*a) != ascii_lower(*b)) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

}  // namespace

const BlendSettingEntry kEntries[] = {
    // ---- HIT (20): entries 0..19 -------------------------------------------
    f("HIT", "fGetUpTime", 0x3F800000u),      // 1.0
    f("HIT", "fKnockDownTime", 0x3E800000u),  // 0.25
    f("HIT", "fMinVelGain", 0x3F733333u),     // 0.95
    f("HIT", "fMinHeirGain", 0x3E99999Au),    // 0.3
    f("HIT", "fHFMin", kZeroBits),            // 0.0
    f("HIT", "fHFMax", kZeroBits),            // 0.0
    f("HIT", "fBFMin", kZeroBits),            // 0.0
    f("HIT", "fBFMax", kZeroBits),            // 0.0
    s("HIT", "sHitSpot", "Bip01 Spine2"),
    s("HIT", "sHitStart", "Bip01 Spine1"),
    s("HIT", "sRHand", "1.0, 1.0"),
    s("HIT", "sRForeArm", "1.0, 1.0"),
    s("HIT", "sRUpperArm", "0.2, 0.5"),
    s("HIT", "sLHand", "1.0, 1.0"),
    s("HIT", "sLForeArm", "1.0, 1.0"),
    s("HIT", "sLUpperArm", "0.2, 0.5"),
    s("HIT", "sSpine2", "0.5, 0.7"),
    s("HIT", "sSpine1", "0.6, 0.8"),
    s("HIT", "sBody", "1.0, 1.0"),
    s("HIT", "sHead", "0.4, 0.6"),

    // ---- QUADHIT (14): entries 20..33 --------------------------------------
    s("QUADHIT", "sHitSpot", "Bip01 Head"),
    s("QUADHIT", "sHitStart", "Bip01 Head"),
    s("QUADHIT", "sRCalf", "1.0, 1.0"),
    s("QUADHIT", "sRThigh", "1.0, 1.0"),
    s("QUADHIT", "sRForeArm", "1.0, 1.0"),
    s("QUADHIT", "sRUpperArm", "1.0, 1.0"),
    s("QUADHIT", "sLCalf", "1.0, 1.0"),
    s("QUADHIT", "sLThigh", "1.0, 1.0"),
    s("QUADHIT", "sLForeArm", "1.0, 1.0"),
    s("QUADHIT", "sLUpperArm", "1.0, 1.0"),
    s("QUADHIT", "sSpine2", "1.0, 1.0"),
    s("QUADHIT", "sSpine1", "1.0, 1.0"),
    s("QUADHIT", "sBody", "1.0, 1.0"),
    s("QUADHIT", "sHead", "0.3, 0.9"),

    // ---- DEFAULT (28): entries 34..61 --------------------------------------
    s("DEFAULT", "sHead", "1.0, 1.0"),
    s("DEFAULT", "sBody", "1.0, 1.0"),
    s("DEFAULT", "sSpine1", "1.0, 1.0"),
    s("DEFAULT", "sSpine2", "1.0, 1.0"),
    s("DEFAULT", "sLUpperArm", "1.0, 1.0"),
    s("DEFAULT", "sLForeArm", "1.0, 1.0"),
    s("DEFAULT", "sLHand", "1.0, 1.0"),
    s("DEFAULT", "sLThigh", "1.0, 1.0"),
    s("DEFAULT", "sLCalf", "1.0, 1.0"),
    s("DEFAULT", "sLFoot", "1.0, 1.0"),
    s("DEFAULT", "sRUpperArm", "1.0, 1.0"),
    s("DEFAULT", "sRForeArm", "1.0, 1.0"),
    // Vanilla shipped "1.0f, 1.0"; normalize the suffix for strict parsing.
    s("DEFAULT", "sRHand", "1.0, 1.0"),
    s("DEFAULT", "sRThigh", "1.0, 1.0"),
    s("DEFAULT", "sRCalf", "1.0, 1.0"),
    s("DEFAULT", "sRFoot", "1.0, 1.0"),
    s("DEFAULT", "sTail", "1.0, 1.0"),
    s("DEFAULT", "sSideWeapon", "1.0, 1.0"),
    s("DEFAULT", "sShield", "1.0, 1.0"),
    s("DEFAULT", "sQuiver", "1.0, 1.0"),
    s("DEFAULT", "sBackWeapon", "1.0, 1.0"),
    s("DEFAULT", "sPonyTail", "1.0, 1.0"),
    f("DEFAULT", "fHighTrans", 0x3D8F5C29u),   // 0.07
    f("DEFAULT", "fHighRot", 0x3BA3D70Au),     // 0.005
    f("DEFAULT", "fLowTrans", 0x3F333333u),    // 0.7
    f("DEFAULT", "fLowRot", 0x3E4CCCCDu),      // 0.2
    f("DEFAULT", "fPassOutTime", 0x3F99999Au), // 1.2
    f("DEFAULT", "fPassOutForce", 0xC1200000u) // -10.0
};

const std::size_t kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

float BlendSettingEntry::float_value() const {
    // Portable bit-pattern decode (no type-punning UB).
    static_assert(sizeof(float) == 4, "IEEE-754 binary32 expected");
    float out;
    std::memcpy(&out, &float_bits, sizeof(out));
    return out;
}

const BlendSettingEntry* find(const char* section, const char* key) {
    if (!section || !key) {
        return nullptr;
    }
    for (std::size_t i = 0; i < kEntryCount; ++i) {
        const BlendSettingEntry& e = kEntries[i];
        if (iequals(e.section, section) && iequals(e.key, key)) {
            return &e;
        }
    }
    return nullptr;
}

bool get_float(const char* section, const char* key, float* out) {
    const BlendSettingEntry* e = find(section, key);
    if (!e || e->type != BlendValueType::kFloat || !out) {
        return false;
    }
    *out = e->float_value();
    return true;
}

bool get_string(const char* section, const char* key, const char** out) {
    const BlendSettingEntry* e = find(section, key);
    if (!e || e->type != BlendValueType::kString || !out) {
        return false;
    }
    *out = e->string_value;
    return true;
}

bool parse_blend_pair(const char* text, float* lo, float* hi) {
    if (!text || !lo || !hi) {
        return false;
    }

    const char* p = text;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    errno = 0;
    char* end = nullptr;
    const float v0 = std::strtof(p, &end);
    if (end == p || errno == ERANGE) {
        return false;
    }
    // C99 strtof may consume a trailing 'f'/'F' suffix; the vanilla format has
    // none, so reject it to keep behavior identical across libc implementations.
    if (*end == 'f' || *end == 'F') {
        return false;
    }

    p = end;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p != ',') {
        return false;
    }
    ++p;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }

    const char* start1 = p;
    errno = 0;
    end = nullptr;
    const float v1 = std::strtof(start1, &end);
    if (end == start1 || errno == ERANGE) {
        return false;
    }
    if (*end == 'f' || *end == 'F') {
        return false;
    }

    p = end;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p != '\0') {
        return false;  // trailing garbage
    }

    *lo = v0;
    *hi = v1;
    return true;
}

}  // namespace blend_settings
}  // namespace oblivion
