#pragma once

// Fatigue (Actor Value 0x0A) formulas - Android port of the original EXE
// constants recovered in RE_EXE_REPORT.md §50.2 / §56.1 / §57.3.
//
// All values are the Oblivion.esm GMST defaults, read from the repo-side
// subset in game/gmst_defaults.h (copied from tmp/re/s50_gmst_full.txt by the
// gmst-table-integration task). Each constant below is a direct reference to
// that table with a static_assert(key exists), so the header holds no
// independent numbers. Runtime overrides from a loaded ESM/MOD are wired
// through gmst::getLive() - published by the AssetManager ESM load path.
// Every function mirrors a specific EXE address noted in the comments.
//
// Not ported yet: the fatigue stage buckets (0x56A300, 0-4) have no consumer
// in the port (the original feeds them to the visibility/animation layer), and
// the automatic knockdown at AV <= 0 (0x600248) waits on the KD apply/recover
// work still open on the EXE side (RE R7). Archery burn follows the weapon
// anim 4/5 rule: bows and staves cost 0 fatigue, so callers skip them.

#include "gmst_defaults.h"

namespace oblivion {
namespace fatigue {

// GMST defaults via gmst_defaults.h (RE §50.2 list + §56.1 Fatigue 式
// [確定] + §57.3). static_assert keeps get()'s 0.0f-missing path out.
inline constexpr float kFatigueBase = gmst::get("fFatigueBase");        // fFatigueBase (0xB37530)
inline constexpr float kFatigueMult = gmst::get("fFatigueMult");        // fFatigueMult (0xB37538)
inline constexpr float kReturnBase = gmst::get("fFatigueReturnBase");   // per second
inline constexpr float kReturnMult = gmst::get("fFatigueReturnMult");   // x End
inline constexpr float kAttackWeaponBase = gmst::get("fFatigueAttackWeaponBase");
inline constexpr float kAttackWeaponMult = gmst::get("fFatigueAttackWeaponMult");  // x dmg
inline constexpr float kPowerAttackPenalty = gmst::get("fPowerAttackFatiguePenalty");
inline constexpr float kBlockSkillBase = gmst::get("fFatigueBlockSkillBase");
inline constexpr float kBlockSkillMult = gmst::get("fFatigueBlockSkillMult");  // x skill
inline constexpr float kBlockMult = gmst::get("fFatigueBlockMult");    // x block amount
inline constexpr float kBlockBase = gmst::get("fFatigueBlockBase");
inline constexpr float kCastBase = gmst::get("fFatigueCastBase");
inline constexpr float kCastMult = gmst::get("fFatigueCastMult");       // x spell cost
inline constexpr float kRunBase = gmst::get("fFatigueRunBase");         // per sec at band 1.0
inline constexpr float kJumpBase = gmst::get("fFatigueJumpBase");
inline constexpr float kJumpMult = gmst::get("fFatigueJumpMult");       // x ratio

static_assert(gmst::hasKey("fFatigueBase"), "gmst_defaults: fFatigueBase");
static_assert(gmst::hasKey("fFatigueMult"), "gmst_defaults: fFatigueMult");
static_assert(gmst::hasKey("fFatigueReturnBase"), "gmst_defaults: fFatigueReturnBase");
static_assert(gmst::hasKey("fFatigueReturnMult"), "gmst_defaults: fFatigueReturnMult");
static_assert(gmst::hasKey("fFatigueAttackWeaponBase"), "gmst_defaults: fFatigueAttackWeaponBase");
static_assert(gmst::hasKey("fFatigueAttackWeaponMult"), "gmst_defaults: fFatigueAttackWeaponMult");
static_assert(gmst::hasKey("fPowerAttackFatiguePenalty"), "gmst_defaults: fPowerAttackFatiguePenalty");
static_assert(gmst::hasKey("fFatigueBlockSkillBase"), "gmst_defaults: fFatigueBlockSkillBase");
static_assert(gmst::hasKey("fFatigueBlockSkillMult"), "gmst_defaults: fFatigueBlockSkillMult");
static_assert(gmst::hasKey("fFatigueBlockMult"), "gmst_defaults: fFatigueBlockMult");
static_assert(gmst::hasKey("fFatigueBlockBase"), "gmst_defaults: fFatigueBlockBase");
static_assert(gmst::hasKey("fFatigueCastBase"), "gmst_defaults: fFatigueCastBase");
static_assert(gmst::hasKey("fFatigueCastMult"), "gmst_defaults: fFatigueCastMult");
static_assert(gmst::hasKey("fFatigueRunBase"), "gmst_defaults: fFatigueRunBase");
static_assert(gmst::hasKey("fFatigueJumpBase"), "gmst_defaults: fFatigueJumpBase");
static_assert(gmst::hasKey("fFatigueJumpMult"), "gmst_defaults: fFatigueJumpMult");

// Fatigue ratio x in [0, 1]. 0x5F4880: GetAV(Fatigue) / v; v <= 0 -> 1.0.
inline float ratio(float current, float maxFatigue) {
    return (maxFatigue > 0.0f) ? current / maxFatigue : 1.0f;
}

// 0x547F00 damage multiplier curve: fFatigueBase - (1 - x) * fFatigueMult.
// Full fatigue -> 1.25, empty -> 0.75.
inline float damageMult(float fatigueRatio) {
    return gmst::getLive("fFatigueBase")
         - (1.0f - fatigueRatio) * gmst::getLive("fFatigueMult");
}

// Combat_CalcMeleeDamage quirk (RE §17.11.1): a ratio of exactly 1.0 bypasses
// the curve and contributes 1.0 (no full-fatigue bonus). Only sub-full
// fatigue receives the curve value.
inline float meleeDamageMult(float fatigueRatio) {
    return (fatigueRatio == 1.0f) ? 1.0f : damageMult(fatigueRatio);
}

// 0x5F2720 recovery per second: fFatigueReturnBase + fFatigueReturnMult * End.
// Mult is 0.0 in the defaults, so the result is a flat 10/s.
inline float recoverPerSecond(float endurance) {
    return gmst::getLive("fFatigueReturnBase") + gmst::getLive("fFatigueReturnMult") * endurance;
}

// 0x547560 attack cost: base + mult * dmg, multiplied by
// fPowerAttackFatiguePenalty on power attacks. Bows/staves (weapon anim 4/5)
// are 0-cost in the original - callers skip them.
inline float attackCost(float weaponDamage, bool powerAttack) {
    const float cost = gmst::getLive("fFatigueAttackWeaponBase")
                     + gmst::getLive("fFatigueAttackWeaponMult") * weaponDamage;
    return powerAttack ? cost * gmst::getLive("fPowerAttackFatiguePenalty") : cost;
}

// 0x547590 block cost: skill * fFatigueBlockSkillMult + fFatigueBlockSkillBase
//             + blockAmount * fFatigueBlockMult + fFatigueBlockBase.
inline float blockCost(float blockSkill, float amount) {
    return blockSkill * gmst::getLive("fFatigueBlockSkillMult")
         + gmst::getLive("fFatigueBlockSkillBase")
         + amount * gmst::getLive("fFatigueBlockMult")
         + gmst::getLive("fFatigueBlockBase");
}

// 0x548DA0 cast cost: fFatigueCastBase + fFatigueCastMult * spellCost.
// Mult is 0.0, so the cost is a flat 15 regardless of the spell.
inline float castCost(float spellCost) {
    return gmst::getLive("fFatigueCastBase")
         + gmst::getLive("fFatigueCastMult") * spellCost;
}

// 0x5FAAE0 athletics band for the run drain: the five skill bands
// (0-19 / 20-39 / 40-59 / 60-79 / 80+) scale the base by
// 1.0 / 0.75 / 0.5 / 0.25 / 0.0.
inline float runAthleticsBand(float athleticsSkill) {
    if (athleticsSkill >= 80.0f) return 0.0f;
    if (athleticsSkill >= 60.0f) return 0.25f;
    if (athleticsSkill >= 40.0f) return 0.5f;
    if (athleticsSkill >= 20.0f) return 0.75f;
    return 1.0f;
}

// Run drain per second: fFatigueRunBase * athletics band.
inline float runCostPerSecond(float athleticsSkill) {
    return gmst::getLive("fFatigueRunBase") * runAthleticsBand(athleticsSkill);
}

// PC jump cost (0x671620): 4 + 4x (x = current fatigue ratio).
inline float jumpCost(float fatigueRatio) {
    return gmst::getLive("fFatigueJumpBase")
         + gmst::getLive("fFatigueJumpMult") * fatigueRatio;
}

// Clamp an AV value into [0, max]. The original apply path clamps at 0 and
// treats <= 0 as the automatic-knockdown trigger (0x600248).
inline float clampValue(float value, float maxValue) {
    if (value < 0.0f) return 0.0f;
    return (value > maxValue) ? maxValue : value;
}

}  // namespace fatigue
}  // namespace oblivion
