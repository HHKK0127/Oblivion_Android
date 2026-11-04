// ============================================================================
// Phase 35: Radiant AI — PackageStack Implementation
// ============================================================================

#include "ai_package.h"
#include <algorithm>

namespace ai {

void PackageStack::pushPackage(const AIPackage& pkg) {
    // Insert sorted by priority (highest first)
    auto it = std::lower_bound(packages.begin(), packages.end(), pkg,
        [](const AIPackage& a, const AIPackage& b) {
            return a.priority > b.priority;  // Higher priority first
        });
    packages.insert(it, pkg);
}

void PackageStack::removePackage(uint32_t packageId) {
    packages.erase(
        std::remove_if(packages.begin(), packages.end(),
            [packageId](const AIPackage& p) { return p.packageId == packageId; }),
        packages.end());
}

void PackageStack::removePackagesByType(PackageType type) {
    packages.erase(
        std::remove_if(packages.begin(), packages.end(),
            [type](const AIPackage& p) { return p.type == type; }),
        packages.end());
}

AIPackage* PackageStack::getActivePackage() {
    // Canonical first-match scan (0x569020): the first package whose
    // conditions evaluated to met wins. No fallback — an empty result means
    // no eligible package and the scheduler idles the NPC.
    for (auto& pkg : packages) {
        if (pkg.conditionsMet) {
            return &pkg;
        }
    }
    return nullptr;
}

const AIPackage* PackageStack::getActivePackage() const {
    for (const auto& pkg : packages) {
        if (pkg.conditionsMet) {
            return &pkg;
        }
    }
    return nullptr;
}

void PackageStack::evaluate(const PackageEvalContext& ctx) {
    // Canonical selection chain per 0x569020: flag predicates -> schedule
    // (day-of-week then [start, start+duration) time window) -> CTDA list.
    // The scan returns the FIRST eligible package; the priority field is a
    // port-only ordering aid (see header comment).
    for (auto& pkg : packages) {
        bool met = pkg.conditions.evaluate(ctx.hourOfDay, ctx.npcPosition,
                                           ctx.npcCellID, ctx.healthPercent,
                                           ctx.magickaPercent,
                                           ctx.staminaPercent);
        if (met && pkg.schedule.enabled &&
            !pkg.schedule.matches(ctx.hourOfDay, ctx.dayOfWeek)) {
            met = false;
        }
        if (met && !pkg.ctdaConditions.empty()) {
            if (ctx.runtime) {
                const game::EvalResult res =
                    game::evaluate_list(&pkg.ctdaConditions, ctx.runAs,
                                        ctx.compareTarget, *ctx.runtime);
                // Skip (unsupported condition function) is treated as pass:
                // documented deviation until the full condition-function
                // bridge lands, so unknown predicates never hard-block AI.
                met = res.passed || res.skip;
            }
            // ctx.runtime == nullptr: game-state bridge pending; the CTDA
            // list is ignored (package passes on its other predicates).
        }
        pkg.conditionsMet = met;
    }
}

} // namespace ai
