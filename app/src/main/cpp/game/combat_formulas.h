#pragma once

// FUN_005FEBF0 combat damage pipeline - confirmed formula fragments ported
// from RE_EXE_REPORT.md §56.1 and tmp/re/s57_combat_full.md §3.1 / §14 / §15.
//
// Only the fragments whose EXE-side math is [確定] (fully reverse engineered
// with GMST defaults matched) live here. Ported so far: block amount
// (0x5474A0), armor factor, the sneak perk multiplier (0x5477F0), and the
// knockback force (0x547690).
//
// Still NOT ported, with the RE status as of s57/s59:
//  - Power-attack multiplier (0x546BA0): direction GMSTs confirmed
//    (fDamagePowerAttackBonus 3.0 / Stand 4.0 / Side-Back-Forward 3.0, perk
//    switches 0x16-0x1a), but the port has no power-attack input path yet
//    (every fatigue::attackCost call passes isPowerAttack=false).
//  - Damage-based KD roll (0x5475D0): computes chance = min(-3*dmg/P, 0.25)
//    from a always-non-negative dmgInt, so it essentially never fires on a
//    normal hit (only ~1% when trunc(damage) == 0). The real KD paths are
//    defender fatigue <= 0 (0x600248) and the Marksman perk rolls
//    (0x60032D / 0x60034F); all of them need a knockdown state machine the
//    port does not have yet.
//  - Knockback application (0x600013-0x6000AF): the force function is
//    ported below, but the EXE applies it as a physics impulse (Havok proxy,
//    fKnockbackTime 1.0). The port only has a death slide with a fixed
//    speed, so living-target knockback wiring is deferred.
//  - Sneak multiplier wiring: 0x5477F0 itself is ported, but FEBF0 only
//    calls it for a successful sneak attack and the port has no sneak state
//    (the script IsSneaking opcode is a stub returning 0).
//  - Per-hit cap (0x5FF2xx): weapon durability term; data incomplete on the
//    port side (weapon.durabilityPercent not populated). Hardcoded to no-op.
//  - Reflect damage (段8, 0x5FF3E0): ported below via applyReflectDamage.
//    AV 0x3b = ReflectDamage validated against §41.3/§56.1.
//  - Speak/stagger (段12, 0x60044D/0x600478): ported below via
//    applyHealthRatioEffects. GMST fCombatSpeakHitThreshold/fCombatSpeakHitChance
//    registered in gmst_defaults.h.
// The port keeps its legacy critical-chance / attack-speed values untouched
// because the RE pass found no EXE-side counterparts for them
// (s57_combat_full.md §5 / §6.2 and §13).
//
// GMST defaults are read from the repo-side subset in game/gmst_defaults.h
// (copied from tmp/re/s50_gmst_full.txt by gmst-table-integration); every
// constant below is a table reference guarded by a static_assert(key exists).
// Runtime overrides from a loaded ESM/MOD are wired through gmst::getLive() -
// published by the AssetManager ESM load path (parseEsmFromMemory / loadEsm).
// Every function below mirrors the EXE address noted in its comment.

#include "fatigue_system.h"
#include "gmst_defaults.h"
#include <cstdlib>

namespace oblivion {
namespace combat {

// 0x5474A0 ComputeBlockAmount GMST defaults (s57_combat_full.md §3.1,
// all [確定]). These are the fBlockSkill*/fBlockAmount* family - NOT the
// fFatigueBlock* family in fatigue_system.h, which drives fatigue cost.
inline constexpr float kBlockReduceSkillBase = gmst::get("fBlockSkillBase");    // 0xB36EE8
inline constexpr float kBlockReduceSkillMult = gmst::get("fBlockSkillMult");    // 0xB36EF0
inline constexpr float kBlockMax = gmst::get("fBlockMax");                      // 0xB36EF8
inline constexpr float kBlockAmountWeapon = gmst::get("fBlockAmountWeaponMult");       // 0xB36F00
inline constexpr float kBlockAmountHandToHand = gmst::get("fBlockAmountHandToHandMult");  // 0xB36F08

// Armor reduction cap: factor = min(AR / 100, fArmorRatingMax)
// (s56_combat_residual.md §armor [確定/一部推定]; the multiply order is
// confirmed by the (1 - ARMOR factor) accumulators at 0x5FF7B5 / 0x5FF908).
inline constexpr float kArmorRatingMax = gmst::get("fArmorRatingMax");  // 0xB36EA0

// 0x547690 knockback GMST defaults (s57_combat_full.md §14.1, all [確定],
// cross-checked against s50_gmst_full.txt slots and the 0x547690 body dump).
inline constexpr float kKnockbackDamageMult = gmst::get("fKnockbackDamageMult");  // 0xB36FC0
inline constexpr float kKnockbackDamageBase = gmst::get("fKnockbackDamageBase");  // 0xB36FB8
inline constexpr float kKnockbackAgilBase = gmst::get("fKnockbackAgilBase");      // 0xB36FA8
inline constexpr float kKnockbackAgilMult = gmst::get("fKnockbackAgilMult");      // 0xB36FB0
inline constexpr float kKnockbackForceMax = gmst::get("fKnockbackForceMax");      // 0xB36FC8

static_assert(gmst::hasKey("fBlockSkillBase"), "gmst_defaults: fBlockSkillBase");
static_assert(gmst::hasKey("fBlockSkillMult"), "gmst_defaults: fBlockSkillMult");
static_assert(gmst::hasKey("fBlockMax"), "gmst_defaults: fBlockMax");
static_assert(gmst::hasKey("fBlockAmountWeaponMult"), "gmst_defaults: fBlockAmountWeaponMult");
static_assert(gmst::hasKey("fBlockAmountHandToHandMult"), "gmst_defaults: fBlockAmountHandToHandMult");
static_assert(gmst::hasKey("fArmorRatingMax"), "gmst_defaults: fArmorRatingMax");
static_assert(gmst::hasKey("fKnockbackDamageMult"), "gmst_defaults: fKnockbackDamageMult");
static_assert(gmst::hasKey("fKnockbackDamageBase"), "gmst_defaults: fKnockbackDamageBase");
static_assert(gmst::hasKey("fKnockbackAgilBase"), "gmst_defaults: fKnockbackAgilBase");
static_assert(gmst::hasKey("fKnockbackAgilMult"), "gmst_defaults: fKnockbackAgilMult");
static_assert(gmst::hasKey("fKnockbackForceMax"), "gmst_defaults: fKnockbackForceMax");
static_assert(gmst::hasKey("fCombatSpeakHitThreshold"), "gmst_defaults: fCombatSpeakHitThreshold");
static_assert(gmst::hasKey("fCombatSpeakHitChance"), "gmst_defaults: fCombatSpeakHitChance");

// 0x547B90 luck-adjusted skill: clamp(skill + 0.4 * luck - 20, 0, 100).
// Both inputs are raw AV values (Block, Luck) - not the /10 bonus form.
inline float luckAdjustedSkill(float skill, float luck) {
    const float d = skill + 0.4f * luck - 20.0f;
    if (d < 0.0f) return 0.0f;
    return (d > 100.0f) ? 100.0f : d;
}

// 0x5474A0 block amount -> the defender reduces incoming damage by
// damage *= (1 - value) (multiply site 0x5FF8D5-0x5FF912, [確定]).
//
//   D    = luckAdjustedSkill(blockSkill, luck)
//   base = 0.01 * D * fBlockSkillMult + fBlockSkillBase
//   M    = weaponBlock ? 0.5 : (handToHand ? 0.25 : 1.0)
//   value = min(curve(fatigueRatio) * base * M, fBlockMax)
//
// curve is the 0x547F00 fatigue curve (fFatigueBase - (1 - x) * fFatigueMult).
// The EXE feeds it 0x5F4880's raw/floor(Fatigue) result (stays near 1.0,
// meaning [推定] per s57 §C2); the port feeds the real residual ratio instead
// so a drained defender blocks noticeably less.
inline float blockAmount(float blockSkill, float luck, float fatigueRatio,
                         bool isWeaponBlock, bool isHandToHand) {
    const float d = luckAdjustedSkill(blockSkill, luck);
    const float base = 0.01f * d * gmst::getLive("fBlockSkillMult")
                       + gmst::getLive("fBlockSkillBase");
    float amountTypeMult = 1.0f;
    if (isWeaponBlock) {
        amountTypeMult = gmst::getLive("fBlockAmountWeaponMult");
    } else if (isHandToHand) {
        amountTypeMult = gmst::getLive("fBlockAmountHandToHandMult");
    }
    const float value =
        oblivion::fatigue::damageMult(fatigueRatio) * base * amountTypeMult;
    const float kBlockMaxLive = gmst::getLive("fBlockMax");
    return (value > kBlockMaxLive) ? kBlockMaxLive : value;
}

// Multiplicative armor reduction factor in [0, 1].
inline float armorFactor(float armorRating) {
    const float factor = armorRating / 100.0f;
    if (factor < 0.0f) return 0.0f;
    const float maxRating = gmst::getLive("fArmorRatingMax");
    return (factor > maxRating) ? maxRating : factor;
}

// 0x5477F0 GetPerkSneakAttackMult (s57 §15.5; branch logic re-verified
// against the raw disassembly in tmp/re/s59_combat_4_out.txt lines 249-342,
// which corrects the s57 prose: weaponType in {-1, 0, 2} selects the MELEE
// table, not 1.0).
//
//   weaponType in {-1, 0, 2} -> fPerkSneakAttackMelee{Rank}Mult (al=1 path)
//   weaponType == 5 (bow)    -> fPerkSneakAttackMarksman{Rank}Mult (al=0)
//   anything else            -> 1.0 (early ret with fld1 still on ST0)
//   rank > 4                 -> 1.0 (0x5478f5 returns the initial fld1)
//
// rank is the Sneak AV 0x1F mastery rank: 0 Novice .. 4 Master. Defaults
// from s50_gmst_full.txt lines 769-778: Melee {4.0, 6, 6, 6, 6},
// Marksman {6.0, 8, 8, 8, 8}. Note fDamageSneakAttackMult (4.0) is NOT
// read anywhere - it only happens to equal the melee novice default.
inline float perkSneakAttackMult(int weaponType, int sneakRank) {
    const bool isMeleePath = (weaponType == -1 || weaponType == 0 || weaponType == 2);
    const bool isMarksmanPath = (weaponType == 5);
    if (!isMeleePath && !isMarksmanPath) return 1.0f;
    if (sneakRank > 4 || sneakRank < 0) return 1.0f;
    if (isMeleePath) {
        // fPerkSneakAttackMelee{Novice,Apprentice,Journeyman,Expert,Master}Mult
        switch (sneakRank) {
            case 0: return gmst::getLive("fPerkSneakAttackMeleeNoviceMult");
            case 1: return gmst::getLive("fPerkSneakAttackMeleeApprenticeMult");
            case 2: return gmst::getLive("fPerkSneakAttackMeleeJourneymanMult");
            case 3: return gmst::getLive("fPerkSneakAttackMeleeExpertMult");
            default: return gmst::getLive("fPerkSneakAttackMeleeMasterMult");
        }
    }
    // fPerkSneakAttackMarksman{Novice,Apprentice,Journeyman,Expert,Master}Mult
    switch (sneakRank) {
        case 0: return gmst::getLive("fPerkSneakAttackMarksmanNoviceMult");
        case 1: return gmst::getLive("fPerkSneakAttackMarksmanApprenticeMult");
        case 2: return gmst::getLive("fPerkSneakAttackMarksmanJourneymanMult");
        case 3: return gmst::getLive("fPerkSneakAttackMarksmanExpertMult");
        default: return gmst::getLive("fPerkSneakAttackMarksmanMasterMult");
    }
}

static_assert(gmst::hasKey("fPerkSneakAttackMeleeNoviceMult"), "gmst_defaults: fPerkSneakAttackMeleeNoviceMult");
static_assert(gmst::hasKey("fPerkSneakAttackMeleeApprenticeMult"), "gmst_defaults: fPerkSneakAttackMeleeApprenticeMult");
static_assert(gmst::hasKey("fPerkSneakAttackMeleeJourneymanMult"), "gmst_defaults: fPerkSneakAttackMeleeJourneymanMult");
static_assert(gmst::hasKey("fPerkSneakAttackMeleeExpertMult"), "gmst_defaults: fPerkSneakAttackMeleeExpertMult");
static_assert(gmst::hasKey("fPerkSneakAttackMeleeMasterMult"), "gmst_defaults: fPerkSneakAttackMeleeMasterMult");
static_assert(gmst::hasKey("fPerkSneakAttackMarksmanNoviceMult"), "gmst_defaults: fPerkSneakAttackMarksmanNoviceMult");
static_assert(gmst::hasKey("fPerkSneakAttackMarksmanApprenticeMult"), "gmst_defaults: fPerkSneakAttackMarksmanApprenticeMult");
static_assert(gmst::hasKey("fPerkSneakAttackMarksmanJourneymanMult"), "gmst_defaults: fPerkSneakAttackMarksmanJourneymanMult");
static_assert(gmst::hasKey("fPerkSneakAttackMarksmanExpertMult"), "gmst_defaults: fPerkSneakAttackMarksmanExpertMult");
static_assert(gmst::hasKey("fPerkSneakAttackMarksmanMasterMult"), "gmst_defaults: fPerkSneakAttackMarksmanMasterMult");

// 0x547690 knockback force (s57 §14.1, [確定]; GMST slots verified in
// s50_gmst_full.txt and the body dump).
//
//   D  = luckAdjustedSkill(agility, luck)
//   Q  = D / curve(x)                    // KD (0x5475D0) uses curve(x) * D -
//                                        // the curve applies inversely here
//   Tk = damage * fKnockbackDamageMult + fKnockbackDamageBase
//   force = Tk * (fKnockbackAgilBase + fKnockbackAgilMult * Q)
//
// Note the EXE clamps the result to fKnockbackForceMax at the CALLER
// (0x600013), not inside this function - compare with minKnockbackForce
// when wiring the application site. damage is the always-non-negative
// accumulated damage (dmgInt = trunc(damage), s57 §15.1); fatigueRatio is
// the target's residual ratio for the 0x547F00 curve.
inline float knockbackForce(float damage, float agility, float luck,
                            float fatigueRatio) {
    const float d = luckAdjustedSkill(agility, luck);
    const float q = d / oblivion::fatigue::damageMult(fatigueRatio);
    const float tk = damage * gmst::getLive("fKnockbackDamageMult") + gmst::getLive("fKnockbackDamageBase");
    return tk * (gmst::getLive("fKnockbackAgilBase") + gmst::getLive("fKnockbackAgilMult") * q);
}

// Caller-side clamp from 0x600013: min(force, fKnockbackForceMax).
inline float minKnockbackForce(float force) {
    const float maxForce = gmst::getLive("fKnockbackForceMax");
    return (force > maxForce) ? maxForce : force;
}

// febf0 段8: Reflect damage (RE §41.3 / §56.1).
// AV index 0x3b = ReflectDamage (0-100%). When the defender has reflect
// active, a fraction of the incoming damage is reflected back to the
// attacker. The player is excluded from being the reflector target.
// Returns the reflected damage amount (0.0 = no reflection).
// The original damage still goes through to the defender unaffected.
inline float applyReflectDamage(float incomingDamage,
                                const CharacterStatus& defender,
                                const CharacterStatus& attacker,
                                bool attackerIsPlayer) {
    if (attackerIsPlayer) return 0.0f;
    auto it = defender.attributes.find("ReflectDamage");
    if (it == defender.attributes.end() || it->second <= 0.0f) return 0.0f;
    const float reflectPct = it->second;
    const float reflected = incomingDamage * (reflectPct / 100.0f);
    return (reflected > 0.0f) ? reflected : 0.0f;
}

// febf0 段12: Speak/stagger check (RE §56.1, lines 9540-9548).
// Called after damage is applied to the target. Checks whether the
// damage exceeds fCombatSpeakHitThreshold * maxHP and whether a random
// roll falls below fCombatSpeakHitChance for a speak line.
// Stagger condition: target has stagger flag, is not busy, frontal 20° cone.
// Both speak and stagger are stubs: speak triggers a log event, stagger
// requires a state machine the port does not have yet.
inline void applyHealthRatioEffects(float damage,
                                    const CharacterStatus& target) {
    const float threshold = gmst::getLive("fCombatSpeakHitThreshold");
    const float chance = gmst::getLive("fCombatSpeakHitChance");
    if (damage > target.maxHealth * threshold) {
        const float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        if (roll < chance) {
            // Speak line trigger (audio hook stub)
            __android_log_print(ANDROID_LOG_DEBUG, "CombatFormulas",
                "Speak line triggered (damage=%.1f, threshold=%.2f*%.1f=%.1f)",
                damage, threshold, target.maxHealth, target.maxHealth * threshold);
        }
    }
}

}  // namespace combat
}  // namespace oblivion
