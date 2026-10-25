#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <glm/glm.hpp>

#include "lod_block_rules.h"

class AssetManager;
class Material;

// ============================================================================
// Original LOD Provider - Phase 65
//
// Phase 50 built its distant terrain from the LAND heightmaps of the loaded
// cells only, so everything past the dense-terrain ring was empty. The retail
// game instead ships pre-baked whole-landscape LOD blocks that cover the whole
// world:
//
//   meshes\landscape\lod\<formID>.<blockX>.<blockY>.32.nif
//   textures\landscapelod\generated\<formID>.<blockX>.<blockY>.32.dds
//   textures\landscapelod\generated\<formID>.<blockX>.<blockY>.32_fn.dds
//
// Each NIF holds one 32 x 32 cell block (131072 x 131072 game units) of
// shrink-wrapped low detail terrain, store: X and Y span 0..131072 and Z (the
// height axis) spans roughly -512..27000. The DDS is a matching 1024x1024 DXT1
// colour map whose UVs line up with the NIF's UV set with no V flip.
//
// This provider indexes those archives, streams in the blocks that cover the
// player, and hands them to DistantLodManager as {@code blockLod} meshes. The
// "_fn" normal maps are skipped: they would double the texture memory of a pass
// that is largely hidden by fog.
// ============================================================================

class OriginalLodProvider {
public:
    static OriginalLodProvider& instance();

    // Original blocks are 32 x 32 cells wide, in cells.
    static constexpr int32_t BLOCK_CELLS = lod_block::kBlockCells;

    // Register every shipped LOD block overlapping
    // [playerPos - coverageDistance, playerPos + coverageDistance] and drop the
    // ones that fell out of range. Returns the number of active blocks.
    //
    // worldspaceFormID is the WRLD form ID (Tamriel = 60); interior cells (0)
    // and worldspaces without shipped LOD keep the caller on its fallback path
    // because this returns 0.
    int update(AssetManager* assetManager, uint32_t worldspaceFormID,
               const glm::vec3& playerPos, float coverageDistance);

    // True when the archives contain LOD blocks for this worldspace. Unlike
    // update() this may be called before any block is streamed in.
    bool isWorldspaceSupported(AssetManager* assetManager, uint32_t worldspaceFormID);

    // Unregister every block, drop the negative caches and the file indexes.
    void reset();

    int getActiveBlockCount() const {
        return static_cast<int>(activeBlocks_.size());
    }
    uint32_t getRegisteredBlockCount() const { return registeredBlocks_; }
    uint32_t getFailedBlockCount() const {
        return static_cast<uint32_t>(failedBlocks_.size());
    }
    uint32_t getLoadedTextureCount() const { return loadedTextures_; }
    uint32_t getFailedTextureCount() const { return failedTextures_; }
    uint32_t getCurrentWorldspace() const { return currentWorldspace_; }

private:
    OriginalLodProvider() = default;
    ~OriginalLodProvider() = default;
    OriginalLodProvider(const OriginalLodProvider&) = delete;
    OriginalLodProvider& operator=(const OriginalLodProvider&) = delete;

    // Archive paths of the mesh and the colour map of one block. Either can be
    // empty when the file is missing from the installation.
    struct BlockPaths {
        std::string mesh;
        std::string texture;
    };

    struct WorldspaceIndex {
        std::unordered_map<uint64_t, BlockPaths> blocks;
        bool built = false;
    };

    struct ActiveBlock {
        std::string meshId;
        // Keeps the GL texture alive: Material owns the texture object, and the
        // AssetManager cache may evict it at any time.
        std::shared_ptr<Material> texture;
    };

    // Block coordinates use game cells, so a block starts at a multiple of
    // BLOCK_CELLS and negative values are valid.
    static uint64_t blockKey(int32_t blockX, int32_t blockY);
    static int32_t blockKeyX(uint64_t key);
    static int32_t blockKeyY(uint64_t key);

    const WorldspaceIndex* getOrBuildIndex(AssetManager* assetManager,
                                           uint32_t worldspaceFormID);
    bool registerBlock(AssetManager* assetManager, uint32_t worldspaceFormID,
                       const BlockPaths& paths, int32_t blockX, int32_t blockY);
    void unregisterActiveBlocks();

    // Built once per worldspace, on demand.
    std::unordered_map<uint32_t, WorldspaceIndex> indices_;
    std::unordered_map<uint64_t, ActiveBlock> activeBlocks_;
    // Blocks whose NIF failed to load. Retrying them every frame would stall
    // the render thread on decompression and logging.
    std::unordered_set<uint64_t> failedBlocks_;

    uint32_t currentWorldspace_ = 0;
    uint32_t registeredBlocks_ = 0;
    uint32_t loadedTextures_ = 0;
    uint32_t failedTextures_ = 0;
};
