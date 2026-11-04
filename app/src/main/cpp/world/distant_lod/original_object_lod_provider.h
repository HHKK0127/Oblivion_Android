// Original-object LOD provider (Phase 68).
//
// Loads the per-cell object-LOD files ("distantlod/<worldspace>_<x>_<y>.lod")
// shipped inside the game's own BSAs and exposes their replicated statics
// (TESObjectSTAT formID + position / Z yaw / uniform scale) keyed by cell.
//
// Format and porting policy are reverse-engineered from the shipped engine:
// docs/RE_EXE_REPORT.md section 54.8 and tmp/re/v255_object_lod_structure.md.
// Each file is a u32 record count followed by `count` variable-length
// records: formId, instance count n, then n position / rotation / scale
// float3 triples (28 bytes per instance). The engine reads only `count`
// records; 11% of the shipped files carry a valid extra record past the
// header count, which the engine silently drops. Section 54.8 item 2 rules
// that the port adopts every record that parses cleanly to EOF instead, so
// this provider walks to end-of-file (see object_lod::parseObjectLodFile).
//
// This is deliberately a SEPARATE system from the landscape OriginalLodProvider:
// landscape blocks are pre-merged NIF meshes keyed by 8x8-cell blocks, while
// object entries are per-cell replicated statics keyed by 1-cell files
// (section 54.8 item 3). The two must not share ActiveBlock-style bookkeeping.
//
// Drawing is deferred: the landscape pipeline (distant_lod_manager /
// lod_block::packGeometry) remaps geometry into block-local coordinates,
// which does not apply to replicated statics placed at absolute world
// positions with per-instance yaw and scale. Until a draw path exists, this
// provider only loads, resolves and exposes data through activeCells();
// renderer wiring can iterate the cells and submit instances later.
//
// FormIDs resolve through oblivion::ESMManager::findStatic(); unresolvable
// formIDs cause the record to be skipped and the cell to remain usable
// (section 54.8 item 5).

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "object_lod_rules.h"

class AssetManager;

namespace oblivion {
class ESMManager;
}

class OriginalObjectLodProvider {
public:
    static OriginalObjectLodProvider& instance();

    // One streaming pass: build/refresh the cell index for the worldspace,
    // register nearby cells (whole-file loads, capped per call) and evict
    // far ones. Returns the number of active cells.
    // worldspaceFormID is the WRLD form of the current exterior (0 = none).
    int update(AssetManager* assetManager,
               const oblivion::ESMManager* esmManager,
               uint32_t worldspaceFormID,
               const glm::vec3& playerPos,
               float coverageDistance);

    // One resolved drawable instance. modelPath is the static's NIF path
    // (lower-case, BSA-relative); yaw is the record's rot[2] (rot[0]/[1]
    // are always zero in the shipped data); scale is uniform.
    struct Instance {
        std::string modelPath;
        glm::vec3 position{0.0f};
        float yaw = 0.0f;
        float scale = 1.0f;
    };

    // Active cells for the draw hook: cell key -> resolved instances.
    const std::unordered_map<uint64_t, std::vector<Instance>>& activeCells() const {
        return activeCells_;
    }

    void reset();

    int getActiveCellCount() const { return static_cast<int>(activeCells_.size()); }
    int getRegisteredCellCount() const { return registeredCells_; }
    int getFailedCellCount() const { return static_cast<int>(failedCells_.size()); }
    int getSkippedEntryCount() const { return skippedEntries_; }
    int getInstanceCount() const { return instanceCount_; }
    uint32_t getCurrentWorldspace() const { return currentWorldspace_; }

    // Cell keys use the same bit-packing as the landscape provider's block
    // keys so downstream code can share key helpers.
    static uint64_t cellKey(int32_t cellX, int32_t cellY) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) |
               static_cast<uint64_t>(static_cast<uint32_t>(cellY));
    }
    static int32_t cellKeyX(uint64_t key) {
        return static_cast<int32_t>(static_cast<uint32_t>(key >> 32));
    }
    static int32_t cellKeyY(uint64_t key) {
        return static_cast<int32_t>(static_cast<uint32_t>(key & 0xFFFFFFFFull));
    }

private:
    OriginalObjectLodProvider() = default;
    ~OriginalObjectLodProvider() = default;
    OriginalObjectLodProvider(const OriginalObjectLodProvider&) = delete;
    OriginalObjectLodProvider& operator=(const OriginalObjectLodProvider&) = delete;

    // Per-worldspace map of cell key -> archive path of its .lod file.
    struct WorldspaceIndex {
        std::unordered_map<uint64_t, std::string> cellPaths;
        bool built = false;
    };

    WorldspaceIndex* getOrBuildIndex(AssetManager* assetManager,
                                     const oblivion::ESMManager* esmManager,
                                     uint32_t worldspaceFormID);
    bool registerCell(AssetManager* assetManager,
                      const oblivion::ESMManager* esmManager,
                      uint64_t key,
                      const std::string& path);
    void unregisterActiveCells();

    std::unordered_map<uint32_t, WorldspaceIndex> indices_;
    std::unordered_map<uint64_t, std::vector<Instance>> activeCells_;
    std::unordered_set<uint64_t> failedCells_;
    uint32_t currentWorldspace_ = 0;
    int registeredCells_ = 0;
    int skippedEntries_ = 0;
    int instanceCount_ = 0;
};
