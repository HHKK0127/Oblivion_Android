#ifndef SYSTEM_BLEND_SETTINGS_H
#define SYSTEM_BLEND_SETTINGS_H

#include <cstddef>

// ============================================================================
// blend_settings.h — Vanilla ragdoll "BlendSettings" default table (62 entries)
//
// Reverse-engineered from Oblivion.exe with Ghidra. Source of truth:
// tmp/re/v64_section7_remaining.md §2 [確定] (2026-11-03).
//
// Vanilla structure:
//   * CRT$XCU initializer array [62] @0xA29084; each slot points at a static
//     init thunk (0x9E44B0 + n*0x60) that does
//     `push &entry[n]; mov ecx, 0xB11C40; call 0x4040E0 (AddSetting)`.
//   * Static entry array @0xB11A24..0xB11C13, stride 8:
//     { u32 float_bits_or_string_ptr; char* "key:Section" name }.
//   * BlendSettings singleton @0xB11C40, vtable 0xA566A4 (Dump slot is a
//     runtime-unreachable no-op — v64 §4 [確定]; no Dump is ported).
//
// Values are stored bit-exact: floats as their IEEE-754 32-bit patterns
// (hex-verified against the .text/.rdata image), strings verbatim — including
// the vanilla typo "1.0f, 1.0" at sRHand:DEFAULT (entry 46, string pool
// @0xA56A08), preserved per the original-text policy (v64 §2.3 / §5).
//
// The registry is case-insensitive in vanilla (INI-style AddSetting), so
// find() compares section/key with ASCII case-insensitive matching.
// ============================================================================

namespace oblivion {
namespace blend_settings {

enum class BlendValueType : unsigned char {
    kFloat = 0,   // f* entries (14 total)
    kString = 1,  // s* entries (48 total): blend-weight pairs "lo, hi" or bone names
};

struct BlendSettingEntry {
    const char* section;         // "HIT" / "QUADHIT" / "DEFAULT"
    const char* key;             // without the ":Section" suffix, e.g. "fGetUpTime"
    BlendValueType type;
    unsigned int float_bits;     // IEEE-754 pattern; meaningful only when kFloat
    const char* string_value;    // verbatim vanilla text; meaningful only kString

    // Decode float_bits to a float without reinterpret-cast UB.
    float float_value() const;
};

// Full default table in vanilla registration order (entries 0..61).
extern const BlendSettingEntry kEntries[];
extern const std::size_t kEntryCount;   // 62

// Case-insensitive lookup; nullptr when absent.
const BlendSettingEntry* find(const char* section, const char* key);

// Typed accessors; false when absent or type mismatched.
bool get_float(const char* section, const char* key, float* out);
bool get_string(const char* section, const char* key, const char** out);

// Strict parser for the vanilla blend-weight pair format "lo, hi".
// Accepts optional surrounding whitespace, exactly one comma separator.
// Returns false (leaving *lo/*hi untouched) for anything else — notably the
// vanilla typo "1.0f, 1.0" is REJECTED here because vanilla's runtime
// tolerance of the trailing 'f' is unverified (v64 §5 follow-up).
bool parse_blend_pair(const char* text, float* lo, float* hi);

}  // namespace blend_settings
}  // namespace oblivion

#endif  // SYSTEM_BLEND_SETTINGS_H
