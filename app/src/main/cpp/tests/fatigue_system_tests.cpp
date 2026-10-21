// Fatigue system tests - see fatigue_system_tests.h for the rationale.

#include "fatigue_system_tests.h"

#include "../game/fatigue_system.h"

#include <cstdio>
#include <cmath>

namespace {

namespace f = oblivion::fatigue;
constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

}  // namespace

void FatigueSystemTests::record(const std::string& name, bool passed,
                                const std::string& msg) {
    results.push_back({name, passed, msg});
    std::printf("[%s] %s", passed ? "PASS" : "FAIL", name.c_str());
    if (!msg.empty()) std::printf(" - %s", msg.c_str());
    std::printf("\n");
}

bool FatigueSystemTests::near(float a, float b, float eps) {
    return std::fabs(a - b) < eps;
}

int FatigueSystemTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int FatigueSystemTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string FatigueSystemTests::getSummary() const {
    return "Fatigue System Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool FatigueSystemTests::runAllTests() {
    results.clear();

    // --- 1. GMST defaults (RE §50.2 measured values) --------------------------
    {
        record("kFatigueBase == 1.25", near(f::kFatigueBase, 1.25f));
        record("kFatigueMult == 0.5", near(f::kFatigueMult, 0.5f));
        record("kReturnBase == 10.0", near(f::kReturnBase, 10.0f));
        record("kReturnMult == 0.0", near(f::kReturnMult, 0.0f));
        record("kAttackWeaponBase == 8.0", near(f::kAttackWeaponBase, 8.0f));
        record("kAttackWeaponMult == 0.1", near(f::kAttackWeaponMult, 0.1f));
        record("kPowerAttackPenalty == 5.0", near(f::kPowerAttackPenalty, 5.0f));
        record("kBlockSkillBase == 5.0", near(f::kBlockSkillBase, 5.0f));
        record("kBlockSkillMult == -0.04", near(f::kBlockSkillMult, -0.04f));
        record("kBlockMult == 1.0", near(f::kBlockMult, 1.0f));
        record("kBlockBase == 0.0", near(f::kBlockBase, 0.0f));
        record("kCastBase == 15.0", near(f::kCastBase, 15.0f));
        record("kCastMult == 0.0", near(f::kCastMult, 0.0f));
        record("kRunBase == 8.0", near(f::kRunBase, 8.0f));
        record("kJumpBase == 4.0", near(f::kJumpBase, 4.0f));
        record("kJumpMult == 4.0", near(f::kJumpMult, 4.0f));
    }

    // --- 2. Ratio (0x5F4880: current / max, max <= 0 -> 1.0) ------------------
    {
        record("ratio(50, 100) == 0.5", near(f::ratio(50.0f, 100.0f), 0.5f));
        record("ratio(0, 100) == 0.0", near(f::ratio(0.0f, 100.0f), 0.0f));
        record("ratio(100, 100) == 1.0", near(f::ratio(100.0f, 100.0f), 1.0f));
        record("ratio(x, 0) == 1.0", near(f::ratio(37.0f, 0.0f), 1.0f));
        record("ratio(negative, max) clamps via caller; raw is negative",
               f::ratio(-5.0f, 100.0f) < 0.0f);
    }

    // --- 3. Damage curve (0x547F00) ------------------------------------------
    {
        record("damageMult(1.0) == 1.25 (full)", near(f::damageMult(1.0f), 1.25f));
        record("damageMult(0.5) == 1.00 (half)", near(f::damageMult(0.5f), 1.00f));
        record("damageMult(0.0) == 0.75 (empty)", near(f::damageMult(0.0f), 0.75f));
        record("damageMult(0.75) == 1.125", near(f::damageMult(0.75f), 1.125f));

        // Combat_CalcMeleeDamage quirk: exactly 1.0 bypasses the curve.
        record("meleeDamageMult(1.0) == 1.0 (exact-full quirk)",
               near(f::meleeDamageMult(1.0f), 1.0f));
        record("meleeDamageMult(0.999) uses the curve",
               near(f::meleeDamageMult(0.999f), f::damageMult(0.999f)));
    }

    // --- 4. Recovery (0x5F2720: flat 10/s, End-independent) ------------------
    {
        record("recoverPerSecond(End 0) == 10", near(f::recoverPerSecond(0.0f), 10.0f));
        record("recoverPerSecond(End 100) == 10 (Mult=0)",
               near(f::recoverPerSecond(100.0f), 10.0f));
    }

    // --- 5. Attack cost (0x547560) -------------------------------------------
    {
        record("attackCost(dmg 10, normal) == 9.0", near(f::attackCost(10.0f, false), 9.0f));
        record("attackCost(dmg 10, power) == 45.0 (x5)", near(f::attackCost(10.0f, true), 45.0f));
        record("attackCost(dmg 0, normal) == 8.0 (base only)", near(f::attackCost(0.0f, false), 8.0f));
        record("attackCost(dmg 20, power) == 50.0", near(f::attackCost(20.0f, true), 50.0f));
    }

    // --- 6. Block cost (0x547590) --------------------------------------------
    {
        // skill 50, full block (x=1): -0.04*50 + 5 + 1*1 + 0 = 4.0
        record("blockCost(skill 50, amount 1) == 4.0", near(f::blockCost(50.0f, 1.0f), 4.0f));
        // skill 100, full block: -4 + 5 + 1 = 2.0
        record("blockCost(skill 100, amount 1) == 2.0", near(f::blockCost(100.0f, 1.0f), 2.0f));
        // skill 0: 5 + 1 = 6.0
        record("blockCost(skill 0, amount 1) == 6.0", near(f::blockCost(0.0f, 1.0f), 6.0f));
    }

    // --- 7. Cast cost (0x548DA0: flat 15, Mult=0) ----------------------------
    {
        record("castCost(0) == 15", near(f::castCost(0.0f), 15.0f));
        record("castCost(100) == 15 (Mult=0)", near(f::castCost(100.0f), 15.0f));
    }

    // --- 8. Run drain (0x5FAAE0: base x athletics band) ----------------------
    {
        record("runAthleticsBand(0) == 1.0", near(f::runAthleticsBand(0.0f), 1.0f));
        record("runAthleticsBand(19) == 1.0", near(f::runAthleticsBand(19.0f), 1.0f));
        record("runAthleticsBand(20) == 0.75", near(f::runAthleticsBand(20.0f), 0.75f));
        record("runAthleticsBand(40) == 0.5", near(f::runAthleticsBand(40.0f), 0.5f));
        record("runAthleticsBand(60) == 0.25", near(f::runAthleticsBand(60.0f), 0.25f));
        record("runAthleticsBand(80) == 0.0", near(f::runAthleticsBand(80.0f), 0.0f));
        record("runAthleticsBand(100) == 0.0", near(f::runAthleticsBand(100.0f), 0.0f));
        record("runCostPerSecond(0) == 8.0", near(f::runCostPerSecond(0.0f), 8.0f));
        record("runCostPerSecond(45) == 4.0", near(f::runCostPerSecond(45.0f), 4.0f));
        record("runCostPerSecond(85) == 0.0 (free run)", near(f::runCostPerSecond(85.0f), 0.0f));
    }

    // --- 9. Jump cost (0x671620: 4 + 4x) -------------------------------------
    {
        record("jumpCost(0) == 4.0", near(f::jumpCost(0.0f), 4.0f));
        record("jumpCost(0.5) == 6.0", near(f::jumpCost(0.5f), 6.0f));
        record("jumpCost(1) == 8.0", near(f::jumpCost(1.0f), 8.0f));
    }

    // --- 10. Clamp helper ----------------------------------------------------
    {
        record("clampValue(-5, 100) == 0", near(f::clampValue(-5.0f, 100.0f), 0.0f));
        record("clampValue(50, 100) == 50", near(f::clampValue(50.0f, 100.0f), 50.0f));
        record("clampValue(150, 100) == 100", near(f::clampValue(150.0f, 100.0f), 100.0f));
    }

    // --- 11. Integration spot checks (typical combat flows) ------------------
    {
        // Fresh attacker (ratio 1.0) gets no melee bonus (exact-full quirk).
        const float freshMult = f::meleeDamageMult(f::ratio(100.0f, 100.0f));
        record("fresh attacker melee mult == 1.0", near(freshMult, 1.0f));

        // A tired attacker at half fatigue still uses the curve value 1.0.
        const float halfMult = f::meleeDamageMult(f::ratio(50.0f, 100.0f));
        record("half-fatigue melee mult == 1.0 (curve midpoint)", near(halfMult, 1.0f));

        // A regular sword hit (dmg 10) drains 9 fatigue.
        const float swordCost = f::attackCost(10.0f, false);
        const float after = f::clampValue(20.0f - swordCost, 100.0f);
        record("20 fatigue - 9 cost clamps to 11", near(after, 11.0f));

        // Draining below zero clamps to 0 (auto-KD threshold).
        const float empty = f::clampValue(5.0f - swordCost, 100.0f);
        record("5 fatigue - 9 cost clamps to 0", near(empty, 0.0f));

        // A power attack (x5) empties a full bar of 45 in one swing at dmg 10.
        const float powerCost = f::attackCost(10.0f, true);  // 45
        const float afterPower = f::clampValue(45.0f - powerCost, 100.0f);
        record("45 fatigue - power cost 45 clamps to 0", near(afterPower, 0.0f));
    }

    return getFailCount() == 0;
}
