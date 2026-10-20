// Combat formula tests - see combat_formulas_tests.h for the rationale.

#include "combat_formulas_tests.h"

#include "../game/combat_formulas.h"

#include <cmath>

namespace {

namespace c = oblivion::combat;
namespace f = oblivion::fatigue;
constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

}  // namespace

void CombatFormulasTests::record(const std::string& name, bool passed,
                                 const std::string& msg) {
    results.push_back({name, passed, msg});
}

bool CombatFormulasTests::near(float a, float b, float eps) {
    return std::fabs(a - b) < eps;
}

int CombatFormulasTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int CombatFormulasTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string CombatFormulasTests::getSummary() const {
    return "Combat Formulas Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool CombatFormulasTests::runAllTests() {
    results.clear();

    // --- 1. GMST defaults (s57_combat_full.md 3.1, all confirmed) ------------
    {
        record("kBlockReduceSkillBase == 0.0", near(c::kBlockReduceSkillBase, 0.0f));
        record("kBlockReduceSkillMult == 1.0", near(c::kBlockReduceSkillMult, 1.0f));
        record("kBlockMax == 0.75", near(c::kBlockMax, 0.75f));
        record("kBlockAmountWeapon == 0.5", near(c::kBlockAmountWeapon, 0.5f));
        record("kBlockAmountHandToHand == 0.25", near(c::kBlockAmountHandToHand, 0.25f));
        record("kArmorRatingMax == 1.0", near(c::kArmorRatingMax, 1.0f));
    }

    // --- 2. Luck-adjusted skill (0x547B90: skill + 0.4*luck - 20, clamped) ---
    {
        record("luckAdjustedSkill(50, 50) == 50", near(c::luckAdjustedSkill(50.0f, 50.0f), 50.0f));
        record("luckAdjustedSkill(0, 0) == 0 (clamped)", near(c::luckAdjustedSkill(0.0f, 0.0f), 0.0f));
        record("luckAdjustedSkill(100, 100) == 100 (clamped)",
               near(c::luckAdjustedSkill(100.0f, 100.0f), 100.0f));
        record("luckAdjustedSkill(10, 50) == 10", near(c::luckAdjustedSkill(10.0f, 50.0f), 10.0f));
        record("luckAdjustedSkill(30, 75) == 40", near(c::luckAdjustedSkill(30.0f, 75.0f), 40.0f));
    }

    // --- 3. Block amount (0x5474A0) - RE s57 13 pin values -------------------
    {
        // Block 50 / Luck 50 / x 1.0: D = 50, base = 0.5, curve = 1.25.
        record("blockAmount(50, 50, 1) == 0.625 (RE pin)",
               near(c::blockAmount(50.0f, 50.0f, 1.0f, false, false), 0.625f));
        record("blockAmount weapon == 0.3125 (RE pin)",
               near(c::blockAmount(50.0f, 50.0f, 1.0f, true, false), 0.3125f));
        record("blockAmount hand-to-hand == 0.15625 (RE pin)",
               near(c::blockAmount(50.0f, 50.0f, 1.0f, false, true), 0.15625f));

        // Caps at fBlockMax: D = 100 -> base = 1.0 -> 1.25 * 1.0 = 1.25 -> 0.75.
        record("blockAmount caps at kBlockMax",
               near(c::blockAmount(100.0f, 100.0f, 1.0f, false, false), 0.75f));

        // Half fatigue: curve(0.5) = 1.0, so the midpoint is exactly base * M.
        record("blockAmount(50, 50, 0.5) == 0.5 (curve midpoint)",
               near(c::blockAmount(50.0f, 50.0f, 0.5f, false, false), 0.5f));

        // Zero skill and luck: D clamps to 0, so no reduction at all.
        record("blockAmount(0, 0, 1) == 0", near(c::blockAmount(0.0f, 0.0f, 1.0f, false, false), 0.0f));

        // Both style flags false keeps the neutral M = 1.0 term.
        record("neutral style uses M = 1.0",
               near(c::blockAmount(50.0f, 50.0f, 1.0f, false, false), 0.625f));
    }

    // --- 4. Armor factor (min(AR/100, 1.0)) ----------------------------------
    {
        record("armorFactor(0) == 0", near(c::armorFactor(0.0f), 0.0f));
        record("armorFactor(40) == 0.4", near(c::armorFactor(40.0f), 0.4f));
        record("armorFactor(100) == 1.0", near(c::armorFactor(100.0f), 1.0f));
        record("armorFactor(137) caps at 1.0", near(c::armorFactor(137.0f), 1.0f));
        record("armorFactor(-5) == 0", near(c::armorFactor(-5.0f), 0.0f));
    }

    // --- 5. Integration spot checks (multiply order in calculateDamage) ------
    {
        // Armor now multiplies: 100 damage through AR 40 -> 60.
        const float afterArmor = 100.0f * (1.0f - c::armorFactor(40.0f));
        record("100 dmg * (1 - 0.4) == 60", near(afterArmor, 60.0f));

        // Full block (weapon style) reduces 100 to 68.75.
        const float blockValue = c::blockAmount(50.0f, 50.0f, 1.0f, true, false);
        const float afterBlock = 100.0f * (1.0f - blockValue);
        record("100 dmg blocked (0.3125) == 68.75", near(afterBlock, 68.75f));

        // Armor then block: 100 through AR 50 and the neutral block amount.
        const float arFactor = c::armorFactor(50.0f);
        const float blockAmt = c::blockAmount(50.0f, 50.0f, 1.0f, false, false);
        const float combined = 100.0f * (1.0f - arFactor) * (1.0f - blockAmt);
        record("100 dmg * (1 - 0.5) * (1 - 0.625) == 18.75", near(combined, 18.75f));

        // The block term consumes the same fatigue curve the melee multiplier
        // uses, so a drained defender and a fresh attacker share one input.
        const float drainedDefender = c::blockAmount(50.0f, 50.0f, f::ratio(0.0f, 100.0f), false, false);
        record("empty-fatigue defender block == 0.375", near(drainedDefender, 0.375f));
    }

    // --- 6. Sneak perk multiplier (0x5477F0, s57 15.5 + raw disassembly) -----
    {
        record("sneak melee novice == 4.0", near(c::perkSneakAttackMult(0, 0), 4.0f));
        record("sneak melee apprentice == 6.0", near(c::perkSneakAttackMult(0, 1), 6.0f));
        record("sneak melee master == 6.0", near(c::perkSneakAttackMult(0, 4), 6.0f));
        record("sneak weaponType -1 uses melee table", near(c::perkSneakAttackMult(-1, 0), 4.0f));
        record("sneak weaponType 2 uses melee table", near(c::perkSneakAttackMult(2, 1), 6.0f));
        record("sneak bow novice == 6.0", near(c::perkSneakAttackMult(5, 0), 6.0f));
        record("sneak bow apprentice == 8.0", near(c::perkSneakAttackMult(5, 1), 8.0f));
        record("sneak bow master == 8.0", near(c::perkSneakAttackMult(5, 4), 8.0f));
        record("sneak weaponType 1 returns 1.0", near(c::perkSneakAttackMult(1, 0), 1.0f));
        record("sneak weaponType 3 returns 1.0", near(c::perkSneakAttackMult(3, 2), 1.0f));
        record("sneak weaponType 4 returns 1.0", near(c::perkSneakAttackMult(4, 4), 1.0f));
        record("sneak rank > 4 returns 1.0", near(c::perkSneakAttackMult(0, 5), 1.0f));
        record("sneak bow rank > 4 returns 1.0", near(c::perkSneakAttackMult(5, 9), 1.0f));
        record("sneak negative rank returns 1.0", near(c::perkSneakAttackMult(0, -1), 1.0f));
    }

    // --- 7. Knockback force (0x547690, s57 14.1 / 15.2) ----------------------
    {
        record("kKnockbackDamageMult == 10.0", near(c::kKnockbackDamageMult, 10.0f));
        record("kKnockbackDamageBase == 50.0", near(c::kKnockbackDamageBase, 50.0f));
        record("kKnockbackAgilBase == 1.0", near(c::kKnockbackAgilBase, 1.0f));
        record("kKnockbackAgilMult == -0.008", near(c::kKnockbackAgilMult, -0.008f));
        record("kKnockbackForceMax == 512.0", near(c::kKnockbackForceMax, 512.0f));

        // D = 50, curve(1.0) = 1.25, Q = 40, Tk = 150, force = 150 * 0.68.
        record("knockback(10, 50, 50, 1) == 102",
               near(c::knockbackForce(10.0f, 50.0f, 50.0f, 1.0f), 102.0f));
        // Zero damage still keeps the fKnockbackDamageBase floor: 50 * 0.68.
        record("knockback(0, 50, 50, 1) == 34",
               near(c::knockbackForce(0.0f, 50.0f, 50.0f, 1.0f), 34.0f));
        // Same target, half fatigue: curve(0.5) = 1.0, Q = 50 -> 0.6.
        record("knockback(10, 50, 50, 0.5) == 90",
               near(c::knockbackForce(10.0f, 50.0f, 50.0f, 0.5f), 90.0f));
        // Zero agility/luck: D clamps to 0, Q = 0, the agil term stays 1.0.
        record("knockback(10, 0, 0, 1) == 150",
               near(c::knockbackForce(10.0f, 0.0f, 0.0f, 1.0f), 150.0f));
        // The caller-side clamp from 0x600013: 1050 * 0.68 = 714 -> 512.
        record("knockback(100, 50, 50, 1) == 714 (raw)",
               near(c::knockbackForce(100.0f, 50.0f, 50.0f, 1.0f), 714.0f));
        record("minKnockbackForce clamps 714 to 512",
               near(c::minKnockbackForce(714.0f), 512.0f));
        record("minKnockbackForce keeps 102",
               near(c::minKnockbackForce(102.0f), 102.0f));
    }

    // --- 8. gmst_defaults table (gmst-table-integration repo subset) ---------
    {
        namespace g = oblivion::gmst;
        // 16 fatigue + 1 time/timing + 22 combat keys = 39 entries.
        // static_assert guards in fatigue_system.h / combat_formulas.h
        // already fail compilation on a missing key; these checks pin the
        // table itself.
        record("gmst table has 39 entries", g::kEntryCount == 39);
        record("gmst has fFatigueBase", g::hasKey("fFatigueBase"));
        record("gmst has fBlockMax", g::hasKey("fBlockMax"));
        record("gmst has fKnockbackForceMax", g::hasKey("fKnockbackForceMax"));
        record("gmst has fPerkSneakAttackMeleeNoviceMult",
               g::hasKey("fPerkSneakAttackMeleeNoviceMult"));
        record("gmst get(fBlockMax) == 0.75", near(g::get("fBlockMax"), 0.75f));
        record("gmst get(unknown) == 0.0 sentinel", near(g::get("noSuchKey"), 0.0f));
        record("gmst has(noSuchKey) == false", !g::hasKey("noSuchKey"));
        // The gameplay constants are table references now - pin two of them
        // against the measured defaults so a table edit shows up here too.
        record("fatigue kFatigueBase == table 1.25",
               near(f::kFatigueBase, g::get("fFatigueBase")) && near(f::kFatigueBase, 1.25f));
        record("combat kArmorRatingMax == table 1.0",
               near(c::kArmorRatingMax, g::get("fArmorRatingMax")) && near(c::kArmorRatingMax, 1.0f));
    }

    // --- 9. GMST runtime overrides (gmst::getLive / setOverride / clearOverride) -
    {
        namespace g = oblivion::gmst;
        g::clearOverrides();

        // No override set -> getLive == get (default).
        record("getLive(fBlockMax) defaults to 0.75",
               near(g::getLive("fBlockMax"), 0.75f));
        record("!hasOverride(fBlockMax)", !g::hasOverride("fBlockMax"));

        // Set override, verify it takes effect.
        g::setOverride("fBlockMax", 0.5f);
        record("getLive(fBlockMax) reads override 0.5",
               near(g::getLive("fBlockMax"), 0.5f));
        record("hasOverride(fBlockMax)", g::hasOverride("fBlockMax"));

        // Gameplay function reflects override.
        record("blockAmount caps at getLive(fBlockMax)=0.5",
               near(c::blockAmount(100.0f, 100.0f, 1.0f, false, false), 0.5f));

        // Clear override, verify revert.
        g::clearOverride("fBlockMax");
        record("getLive(fBlockMax) reverts to 0.75",
               near(g::getLive("fBlockMax"), 0.75f));
        record("!hasOverride(fBlockMax) after clear", !g::hasOverride("fBlockMax"));

        // Override an unknown key is silently ignored.
        g::clearOverrides();
        g::setOverride("noSuchKey", 99.0f);
        record("getLive(unknown) == 0.0 sentinel", near(g::getLive("noSuchKey"), 0.0f));

        // Function still works with default after all cleared.
        record("armorFactor(60) == 0.6 after override clear",
               near(c::armorFactor(60.0f), 0.6f));
    }

    return getFailCount() == 0;
}
