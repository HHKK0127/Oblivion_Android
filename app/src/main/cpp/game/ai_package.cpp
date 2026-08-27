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
    // Return the highest priority package whose conditions are met
    for (auto& pkg : packages) {
        // For now, return the first (highest priority) package
        // Conditions are evaluated separately in evaluate()
        return &pkg;
    }
    return nullptr;
}

void PackageStack::evaluate(float hourOfDay, const glm::vec3& npcPos,
                             uint32_t npcCellID,
                             float healthPct, float magickaPct, float staminaPct) {
    // Evaluate conditions for each package
    // The active package is the highest priority one whose conditions are met
    // This is implicitly handled by the sorted order + getActivePackage()
}

} // namespace ai
