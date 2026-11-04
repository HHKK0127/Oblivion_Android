// Implementation of the original-object LOD provider (Phase 68).
//
// See original_object_lod_provider.h for the contract and docs/RE_EXE_REPORT.md
// section 54.8 for the porting policy. Every rule implemented here (EOF-walk,
// per-entry skip on unresolved formIDs, yaw = rot[2], raw scale) comes from
// the reverse-engineered shipped engine, not from guesswork.

#include "original_object_lod_provider.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "../../assets/asset_manager.h"
#include "../../assets/esm_reader.h"
#include "distant_lod_manager.h"

namespace {

// The shipped archives store the folder lower-case; both separator styles
// appear in the AssetManager file index depending on the archive source.
constexpr const char* kLodPrefixes[] = { "distantlod\\", "distantlod/" };

// One Oblivion exterior cell is 4096 units on a side; the .lod positions are
// world absolute, so the streaming window is derived from the cell grid.
constexpr float kCellSize = 4096.0f;

// Whole-file loads are cheap (40..1072 bytes) but each one also resolves
// every formID through the ESM, so the per-update budget stays modest.
constexpr int kMaxCellsPerUpdate = 16;

// Guard against a pathological coverage distance blowing up the scan.
constexpr int32_t kMaxCellSpanPerAxis = 64;

std::string toLowerAscii(const std::string& value) {
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool endsWithLod(const std::string& name) {
    constexpr const char* kExt = ".lod";
    if (name.size() < 4) {
        return false;
    }
    std::string tail = name.substr(name.size() - 4);
    std::transform(tail.begin(), tail.end(), tail.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return tail == kExt;
}

float cellCenterX(int32_t cellX) {
    return (static_cast<float>(cellX) + 0.5f) * kCellSize;
}

float cellCenterY(int32_t cellY) {
    return (static_cast<float>(cellY) + 0.5f) * kCellSize;
}

void clampRange(int32_t& lo, int32_t& hi, int32_t center) {
    if (hi - lo + 1 > kMaxCellSpanPerAxis) {
        lo = center - kMaxCellSpanPerAxis / 2;
        hi = lo + kMaxCellSpanPerAxis - 1;
    }
}

}  // namespace

OriginalObjectLodProvider& OriginalObjectLodProvider::instance() {
    static OriginalObjectLodProvider provider;
    return provider;
}

OriginalObjectLodProvider::WorldspaceIndex*
OriginalObjectLodProvider::getOrBuildIndex(AssetManager* assetManager,
                                           const oblivion::ESMManager* esmManager,
                                           uint32_t worldspaceFormID) {
    if (!assetManager || !esmManager || worldspaceFormID == 0) {
        return nullptr;
    }

    auto it = indices_.find(worldspaceFormID);
    if (it != indices_.end() && it->second.built) {
        return &it->second;
    }

    // The archives are opened during startup. Building the index before that
    // would cache an empty result for the rest of the session.
    if (assetManager->getArchiveCount() == 0) {
        LOGW_DLOD("Object LOD index for worldspace %u deferred: no archives",
                  worldspaceFormID);
        return nullptr;
    }

    // The file names carry the worldspace by its WRLD editor ID ("Tamriel",
    // "seworld"), so the ESM record has to be available before the index is
    // marked built; the ESM may still be loading on the first frames.
    const oblivion::WorldData* world = esmManager->findWorld(worldspaceFormID);
    if (!world || world->editorID.empty()) {
        return nullptr;
    }
    const std::string worldspaceName = toLowerAscii(world->editorID);

    std::vector<std::string> candidates;
    for (const char* prefix : kLodPrefixes) {
        const std::vector<std::string> found = assetManager->findFiles(prefix);
        candidates.insert(candidates.end(), found.begin(), found.end());
    }
    if (candidates.empty()) {
        // Some archive indexes normalise the folder differently; fall back to
        // a whole-archive extension scan so the data is not silently lost.
        candidates = assetManager->findFilesByExtension(".lod");
    }

    WorldspaceIndex& index = indices_[worldspaceFormID];
    for (const std::string& path : candidates) {
        if (!endsWithLod(path)) {
            continue;
        }
        std::string fileWorldspace;
        int32_t cellX = 0;
        int32_t cellY = 0;
        if (!object_lod::parseCellName(path, fileWorldspace, cellX, cellY)) {
            continue;
        }
        if (toLowerAscii(fileWorldspace) != worldspaceName) {
            continue;
        }
        index.cellPaths[cellKey(cellX, cellY)] = path;
    }
    index.built = true;

    LOGI_DLOD("Object LOD index for worldspace %u (%s): %zu cells",
              worldspaceFormID, world->editorID.c_str(), index.cellPaths.size());
    return &index;
}

int OriginalObjectLodProvider::update(AssetManager* assetManager,
                                      const oblivion::ESMManager* esmManager,
                                      uint32_t worldspaceFormID,
                                      const glm::vec3& playerPos,
                                      float coverageDistance) {
    if (currentWorldspace_ != worldspaceFormID) {
        // Cell keys are only unique inside one worldspace; the "seworld" and
        // "Tamriel" grids overlap completely.
        unregisterActiveCells();
        failedCells_.clear();
        currentWorldspace_ = worldspaceFormID;
    }

    WorldspaceIndex* index = getOrBuildIndex(assetManager, esmManager, worldspaceFormID);
    if (!index || index->cellPaths.empty()) {
        unregisterActiveCells();
        return 0;
    }

    const int32_t playerCellX = static_cast<int32_t>(std::floor(playerPos.x / kCellSize));
    const int32_t playerCellY = static_cast<int32_t>(std::floor(playerPos.z / kCellSize));
    int32_t minCellX = static_cast<int32_t>(std::floor((playerPos.x - coverageDistance) / kCellSize));
    int32_t maxCellX = static_cast<int32_t>(std::floor((playerPos.x + coverageDistance) / kCellSize));
    int32_t minCellY = static_cast<int32_t>(std::floor((playerPos.z - coverageDistance) / kCellSize));
    int32_t maxCellY = static_cast<int32_t>(std::floor((playerPos.z + coverageDistance) / kCellSize));
    clampRange(minCellX, maxCellX, playerCellX);
    clampRange(minCellY, maxCellY, playerCellY);

    std::vector<uint64_t> needed;
    needed.reserve(static_cast<size_t>(maxCellX - minCellX + 1) *
                   static_cast<size_t>(maxCellY - minCellY + 1));
    for (int32_t y = minCellY; y <= maxCellY; ++y) {
        for (int32_t x = minCellX; x <= maxCellX; ++x) {
            const uint64_t key = cellKey(x, y);
            if (index->cellPaths.find(key) != index->cellPaths.end()) {
                needed.push_back(key);
            }
        }
    }

    // Evict cells that left the window before loading new ones so the steady
    // state stays close to the window size.
    std::unordered_set<uint64_t> neededSet(needed.begin(), needed.end());
    for (auto active = activeCells_.begin(); active != activeCells_.end();) {
        if (neededSet.find(active->first) == neededSet.end()) {
            active = activeCells_.erase(active);
        } else {
            ++active;
        }
    }

    struct Candidate {
        uint64_t key;
        float distanceSq;
    };
    std::vector<Candidate> candidates;
    for (const uint64_t key : needed) {
        if (activeCells_.find(key) != activeCells_.end() ||
            failedCells_.find(key) != failedCells_.end()) {
            continue;
        }
        const int32_t x = cellKeyX(key);
        const int32_t y = cellKeyY(key);
        const float dx = cellCenterX(x) - playerPos.x;
        const float dy = cellCenterY(y) - playerPos.z;
        candidates.push_back({ key, dx * dx + dy * dy });
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.distanceSq < b.distanceSq;
              });

    const int loadCount = std::min(kMaxCellsPerUpdate, static_cast<int>(candidates.size()));
    for (int i = 0; i < loadCount; ++i) {
        const uint64_t key = candidates[i].key;
        const auto pathIt = index->cellPaths.find(key);
        if (pathIt == index->cellPaths.end()) {
            continue;
        }
        if (!registerCell(assetManager, esmManager, key, pathIt->second)) {
            failedCells_.insert(key);
        }
    }

    return static_cast<int>(activeCells_.size());
}

bool OriginalObjectLodProvider::registerCell(AssetManager* assetManager,
                                             const oblivion::ESMManager* esmManager,
                                             uint64_t key,
                                             const std::string& path) {
    std::vector<uint8_t> fileData = assetManager->loadFileData(path);
    if (fileData.empty()) {
        LOGW_DLOD("Object LOD cell missing from the archives: %s", path.c_str());
        return false;
    }

    object_lod::ObjectLodCell cell;
    object_lod::ObjectLodParseStats stats;
    if (!object_lod::parseObjectLodFile(fileData, cell, &stats)) {
        LOGW_DLOD("Object LOD cell failed to parse: %s", path.c_str());
        return false;
    }
    if (!stats.headerCountMatches) {
        // Section 54.8 item 2: the shipped engine drops the extra records;
        // the port keeps them, so log the divergence instead of discarding.
        LOGI_DLOD("Object LOD cell %s carries %zu records past the header "
                  "count %u (%zu trailing bytes)",
                  path.c_str(), stats.adoptedRecords, stats.headerCount,
                  stats.trailingBytes);
    }

    cell.cellX = cellKeyX(key);
    cell.cellY = cellKeyY(key);

    std::vector<Instance> instances;
    int skipped = 0;
    for (const object_lod::ObjectLodEntry& entry : cell.entries) {
        const oblivion::StaticData* staticData = esmManager->findStatic(entry.formId);
        if (!staticData || staticData->modelPath.empty()) {
            // Section 54.8 item 5: an unresolvable formID drops the record
            // and the rest of the cell stays usable.
            ++skipped;
            continue;
        }
        for (const object_lod::ObjectLodInstance& inst : entry.instances) {
            Instance out;
            out.modelPath = staticData->modelPath;
            out.position = inst.position;
            out.yaw = inst.zRotation;
            out.scale = inst.scale;
            instances.push_back(std::move(out));
        }
    }
    skippedEntries_ += skipped;

    instanceCount_ += static_cast<int>(instances.size());
    activeCells_[key] = std::move(instances);
    ++registeredCells_;

    LOGI_DLOD("Object LOD cell %d,%d registered (%zu instances, %d records skipped): %s",
              cell.cellX, cell.cellY, activeCells_[key].size(), skipped, path.c_str());
    return true;
}

void OriginalObjectLodProvider::unregisterActiveCells() {
    // No GL resources are registered yet: the draw path is deferred, so the
    // eviction is a plain map clear.
    activeCells_.clear();
}

void OriginalObjectLodProvider::reset() {
    unregisterActiveCells();
    failedCells_.clear();
    indices_.clear();
    currentWorldspace_ = 0;
    registeredCells_ = 0;
    skippedEntries_ = 0;
    instanceCount_ = 0;
    LOGI_DLOD("Object LOD provider reset");
}
