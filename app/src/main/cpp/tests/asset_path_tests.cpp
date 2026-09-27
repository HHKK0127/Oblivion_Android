// Asset path resolution tests - see asset_path_tests.h for the rationale.

#include "asset_path_tests.h"

#include "../assets/asset_path_resolver.h"

#include <algorithm>
#include <chrono>

namespace {

bool contains(const std::vector<std::string>& list, const std::string& value) {
    return std::find(list.begin(), list.end(), value) != list.end();
}

std::vector<std::string> candidatesFor(const std::string& raw) {
    std::string normalized = raw;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return buildAssetPathCandidates(normalized);
}

}  // namespace

void AssetPathTests::record(const std::string& name, bool passed,
                            const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int AssetPathTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int AssetPathTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string AssetPathTests::getSummary() const {
    return "Asset Path Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool AssetPathTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    // 1. The regression: an ESM model path must gain the Meshes root, which is
    //    the only spelling the real Oblivion - Meshes.bsa stores.
    {
        const auto c = candidatesFor("Dungeons\\Misc\\Triggers\\TrigZone01.NIF");
        record("nif keeps the original spelling as first candidate",
               !c.empty() && c[0] == "Dungeons/Misc/Triggers/TrigZone01.NIF");
        record("nif gains the meshes root",
               contains(c, "meshes/Dungeons/Misc/Triggers/TrigZone01.NIF"));
        record("meshes is the preferred nif root",
               c.size() > 1 && c[1] == "meshes/Dungeons/Misc/Triggers/TrigZone01.NIF");
    }

    // 2. Textures resolve the same way (Terrain textured: 0 of 9 came from this).
    {
        const auto c = candidatesFor("Landscape\\Grass01.dds");
        record("dds gains the textures root",
               contains(c, "textures/Landscape/Grass01.dds"));
        record("textures is the preferred dds root",
               c.size() > 1 && c[1] == "textures/Landscape/Grass01.dds");
    }

    // 3. Sound files live under Sound\.
    {
        const auto c = candidatesFor("fx\\env\\dungeon.wav");
        record("wav gains the sound root", contains(c, "sound/fx/env/dungeon.wav"));
    }

    // 4. SpeedTree files live under Trees\ - and not under Meshes\.
    {
        const auto c = candidatesFor("TreeCottonwoodSU.spt");
        record("spt gains the trees root", contains(c, "trees/TreeCottonwoodSU.spt"));
        record("trees is the preferred spt root",
               c.size() > 1 && c[1] == "trees/TreeCottonwoodSU.spt");
    }

    // 5. An already-rooted path is returned untouched: prefixing it again would
    //    produce "meshes/meshes/..." and waste a lookup on every request.
    {
        const auto c = candidatesFor("Meshes\\Architecture\\IC\\ICWall01.NIF");
        record("rooted path yields exactly one candidate", c.size() == 1);
        record("rooted path is unchanged",
               c.size() == 1 && c[0] == "Meshes/Architecture/IC/ICWall01.NIF");
    }

    // 6. Root detection is case-insensitive.
    {
        const auto c = candidatesFor("MESHES\\Architecture\\ICWall01.NIF");
        record("root detection ignores case", c.size() == 1);
    }

    // 7. The loose-file fallback keeps priority: the original spelling is always
    //    tried before any prefixed variant.
    {
        const auto c = candidatesFor("Clutter\\Wine\\WineRed01.NIF");
        record("original spelling precedes prefixed variants",
               c.size() > 1 && c[0] == "Clutter/Wine/WineRed01.NIF");
    }

    // 8. Every known root is offered for a mesh, so a mod that ships a model
    //    outside Meshes\ still resolves.
    {
        const auto c = candidatesFor("Characters\\Imperial\\Male.nif");
        bool allRoots = true;
        for (const auto& root : assetRootFolders()) {
            allRoots = allRoots && contains(c, root + "/Characters/Imperial/Male.nif");
        }
        record("every known root is offered", allRoots);
    }

    // 9. Extension matching is case-insensitive and extensionless paths still
    //    get a mesh-first safety net.
    {
        const auto c = candidatesFor("Dungeons\\Misc\\Thing.NIF");
        record("uppercase extension still prefers meshes",
               c.size() > 1 && c[1] == "meshes/Dungeons/Misc/Thing.NIF");

        const auto noExt = candidatesFor("Some\\Odd\\Asset");
        record("extensionless path still offers the meshes root",
               contains(noExt, "meshes/Some/Odd/Asset"));
    }

    // 10. A bare file name with no folder is still expandable.
    {
        const auto c = candidatesFor("Marker_Prison.nif");
        record("bare name gains the meshes root",
               contains(c, "meshes/Marker_Prison.nif"));
    }

    // 11. The candidate list is never empty and never duplicates an entry.
    {
        const auto c = candidatesFor("a\\b\\c.nif");
        std::vector<std::string> sorted(c);
        std::sort(sorted.begin(), sorted.end());
        const bool unique = std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
        record("candidates are never empty", !c.empty());
        record("candidates contain no duplicates", unique);
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    record("suite completed", true, "", totalMs);

    return getFailCount() == 0;
}
