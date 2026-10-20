#pragma once

// GMST float subset consumed by the port - repo-side source of truth.
//
// Source: RE_EXE_REPORT.md 50.8 (all 2304 GMST entries resolved during the
// RE pass) as recorded in tmp/re/s50_gmst_full.txt (gitignored). This header
// copies the "needed subset" the task gmst-table-integration calls for: every
// float GMST the port's gameplay headers consume today (fatigue_system.h,
// combat_formulas.h). Key names are the ESM editor IDs (first character is
// the value type), which is also what ESMManager::findGameSetting() takes.
//
// Defaults here are the Oblivion.esm default values measured by the RE pass
// (slot VA + f32 default pair recorded per row in s50_gmst_full.txt). The
// gameplay headers reference these entries through get() + a static_assert
// hasKey() guard, so a missing key is a compile error and the headers hold
// no independent copies of the numbers.
//
// Runtime overrides from a loaded ESM/MOD are wired through getLive() --
// published by AssetManager's ESM load path (parseEsmFromMemory / loadEsm)
// after each successful parse. A worker-thread parse can safely publish while
// the render thread reads because each override cell uses atomic release/acquire
// ordering on the set flag (value.store relaxed -> set.store release;
// set.load acquire -> value.load relaxed). getLive() checks the override first,
// then falls back to the compile-time get().
//

#include <atomic>
#include <cstddef>

namespace oblivion {
namespace gmst {

struct Entry {
    const char* key;
    float value;
};

// s50_gmst_full.txt subset - keep sorted the way consumers read it (fatigue
// family first, then combat), not alphabetically.
inline constexpr Entry kEntries[] = {
    // --- fatigue_system.h consumers (RE §50.2 / §56.1 / §57.3) ---
    {"fFatigueBase", 1.25f},                 // 0xB37530
    {"fFatigueMult", 0.5f},                  // 0xB37538
    {"fFatigueReturnBase", 10.0f},           // 0xB37540 (per second)
    {"fFatigueReturnMult", 0.0f},            // 0xB37548 (x End)
    {"fFatigueAttackWeaponBase", 8.0f},      // 0xB36FD8
    {"fFatigueAttackWeaponMult", 0.1f},      // 0xB36FE0 (x dmg)
    {"fPowerAttackFatiguePenalty", 5.0f},    // 0xB36FE8
    {"fFatigueBlockSkillBase", 5.0f},        // 0xB37000
    {"fFatigueBlockSkillMult", -0.04f},      // 0xB37008 (x skill)
    {"fFatigueBlockMult", 1.0f},             // 0xB36FF8 (x block amount)
    {"fFatigueBlockBase", 0.0f},             // 0xB36FF0
    {"fFatigueCastBase", 15.0f},             // 0xB37F40
    {"fFatigueCastMult", 0.0f},              // 0xB37F48 (x spell cost)
    {"fFatigueRunBase", 8.0f},               // 0xB37550 (per sec at band 1.0)
    {"fFatigueJumpBase", 4.0f},              // 0xB37560
    {"fFatigueJumpMult", 4.0f},              // 0xB37568 (x ratio)

    // --- time / timing (RE §54.3) ---
    {"fGlobalTimeMultiplier", 1.0f},           // 0xB06704

    // --- combat_formulas.h consumers (RE §56.1 / §14 / §15) ---
    {"fArmorRatingMax", 1.0f},               // 0xB36EA0
    {"fBlockSkillBase", 0.0f},               // 0xB36EE8
    {"fBlockSkillMult", 1.0f},               // 0xB36EF0
    {"fBlockMax", 0.75f},                    // 0xB36EF8
    {"fBlockAmountWeaponMult", 0.5f},        // 0xB36F00
    {"fBlockAmountHandToHandMult", 0.25f},   // 0xB36F08
    {"fKnockbackAgilBase", 1.0f},            // 0xB36FA8
    {"fKnockbackAgilMult", -0.008f},         // 0xB36FB0
    {"fKnockbackDamageBase", 50.0f},         // 0xB36FB8
    {"fKnockbackDamageMult", 10.0f},         // 0xB36FC0
    {"fKnockbackForceMax", 512.0f},          // 0xB36FC8
    {"fKnockbackTime", 1.0f},                // 0xB36FD0 (impulse time, unwired)
    {"fPerkSneakAttackMeleeNoviceMult", 4.0f},        // 0xB37338
    {"fPerkSneakAttackMarksmanNoviceMult", 6.0f},     // 0xB37340
    {"fPerkSneakAttackMeleeApprenticeMult", 6.0f},    // 0xB37348
    {"fPerkSneakAttackMarksmanApprenticeMult", 8.0f}, // 0xB37350
    {"fPerkSneakAttackMeleeJourneymanMult", 6.0f},    // 0xB37358
    {"fPerkSneakAttackMarksmanJourneymanMult", 8.0f}, // 0xB37360
    {"fPerkSneakAttackMeleeExpertMult", 6.0f},        // 0xB37368
    {"fPerkSneakAttackMarksmanExpertMult", 8.0f},     // 0xB37370
    {"fPerkSneakAttackMeleeMasterMult", 6.0f},        // 0xB37378
{"fPerkSneakAttackMarksmanMasterMult", 8.0f},        // 0xB37380

    // --- febf0 tail: speak/stagger (RE §56.1, 0x60044D/0x600478) ---
    {"fCombatSpeakHitThreshold", 0.10f},           // 0xB36F58
    {"fCombatSpeakHitChance", 0.35f},             // 0xB36F50
};

inline constexpr std::size_t kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

constexpr bool equal(const char* a, const char* b) {
    // C++17 has no constexpr strcmp; compare byte by byte.
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

constexpr bool hasKey(const char* key) {
    for (const Entry& e : kEntries) {
        if (equal(e.key, key)) return true;
    }
    return false;
}

// Returns the default for key, or 0.0f when missing. Consumers guard their
// lookup with static_assert(hasKey(...)) so the 0.0f path never compiles
// into gameplay constants - a key must exist here first.
constexpr float get(const char* key) {
    for (const Entry& e : kEntries) {
        if (equal(e.key, key)) return e.value;
    }
    return 0.0f;
}

// --- runtime overrides (ESM/MOD) -------------------------------------------
// Published by the loader after each ESM/plugin parse (AssetManager ESM load
// path). Readers use getLive() at the gameplay call sites; get() stays the
// compile-time default used by the k* pins and the static_assert guards.
// Each cell is atomic so a parse on a worker thread can publish while
// gameplay reads: value.store(relaxed) happens-before set.store(release),
// readers mirror with set.load(acquire) -> value.load(relaxed).

struct OverrideCell {
    std::atomic<float> value{0.0f};
    std::atomic<bool> set{false};
};

inline OverrideCell g_overrides[kEntryCount];

constexpr std::size_t indexOf(const char* key) {
    std::size_t i = 0;
    for (const Entry& e : kEntries) {
        if (equal(e.key, key)) return i;
        ++i;
    }
    return kEntryCount;
}

inline void setOverride(const char* key, float value) {
    const std::size_t i = indexOf(key);
    if (i == kEntryCount) return;
    g_overrides[i].value.store(value, std::memory_order_relaxed);
    g_overrides[i].set.store(true, std::memory_order_release);
}

inline void clearOverride(const char* key) {
    const std::size_t i = indexOf(key);
    if (i == kEntryCount) return;
    g_overrides[i].set.store(false, std::memory_order_release);
}

inline void clearOverrides() {
    for (std::size_t i = 0; i < kEntryCount; ++i) {
        g_overrides[i].set.store(false, std::memory_order_release);
    }
}

inline bool hasOverride(const char* key) {
    const std::size_t i = indexOf(key);
    return i != kEntryCount
        && g_overrides[i].set.load(std::memory_order_acquire);
}

// Runtime lookup: override when the loaded ESM/MOD defines the key, the
// measured default otherwise. Unknown keys mirror get()'s 0.0f sentinel.
inline float getLive(const char* key) {
    const std::size_t i = indexOf(key);
    if (i == kEntryCount) return 0.0f;
    if (g_overrides[i].set.load(std::memory_order_acquire)) {
        return g_overrides[i].value.load(std::memory_order_relaxed);
    }
    return get(key);
}

}  // namespace gmst
}  // namespace oblivion
