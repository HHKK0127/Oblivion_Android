#include "original_lod_provider.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>
#include <vector>

#include "../../assets/asset_manager.h"
#include "../../assets/nif_parser.h"
#include "../../assets/nif_types.h"
#include "../world_data.h"
#include "distant_lod_manager.h"

namespace {

constexpr const char* kMeshPrefix = "meshes\\landscape\\lod\\";
constexpr const char* kTexturePrefix = "textures\\landscapelod\\generated\\";

// Strict integer parse: the whole token has to be numeric, so a stray file such
// as "60x.00.00.32.nif" is rejected instead of silently parsed as 60.
bool parseIntStrict(const std::string& text, long& out) {
    if (text.empty()) {
        return false;
    }
    size_t consumed = 0;
    try {
        out = std::stol(text, &consumed);
    } catch (const std::exception&) {
        return false;
    }
    return consumed == text.size();
}

// The retail naming is "<formID>.<blockX>.<blockY>.32.<ext>" with the block
// coordinates written as plain signed decimals, except that zero is stored as
// "00" (the shipped files are e.g. "60.00.00.32.nif", "60.-32.64.32.nif").
bool parseBlockName(const std::string& path, uint32_t worldspaceFormID,
                    int32_t& outBlockX, int32_t& outBlockY) {
    const size_t separator = path.find_last_of("\\/");
    const std::string name =
        (separator == std::string::npos) ? path : path.substr(separator + 1);

    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t dot = name.find('.', start);
        if (dot == std::string::npos) {
            parts.push_back(name.substr(start));
            break;
        }
        parts.push_back(name.substr(start, dot - start));
        start = dot + 1;
    }
    if (parts.size() != 5) {
        return false;
    }

    long formID = 0;
    long blockX = 0;
    long blockY = 0;
    long blockCells = 0;
    if (!parseIntStrict(parts[0], formID) ||
        !parseIntStrict(parts[1], blockX) ||
        !parseIntStrict(parts[2], blockY) ||
        !parseIntStrict(parts[3], blockCells)) {
        return false;
    }
    if (formID <= 0 || static_cast<uint32_t>(formID) != worldspaceFormID) {
        return false;
    }
    // Only the 32 cell block size shipped by the game is handled.
    if (blockCells != OriginalLodProvider::BLOCK_CELLS) {
        return false;
    }

    outBlockX = static_cast<int32_t>(blockX);
    outBlockY = static_cast<int32_t>(blockY);
    return true;
}

// Strips the extension and the "_fn" suffix of a normal map so the stem can be
// matched against the mesh name.
std::string textureStem(const std::string& path) {
    std::string stem = path;
    const size_t separator = stem.find_last_of("\\/");
    if (separator != std::string::npos) {
        stem.erase(0, separator + 1);
    }
    const size_t dot = stem.find_last_of('.');
    if (dot != std::string::npos) {
        stem.erase(dot);
    }
    return stem;
}

// Rounds a cell coordinate down to the block that contains it. C++ integer
// division truncates towards zero, which would be wrong for negative cells.
int32_t floorToBlock(int32_t cell) {
    const int32_t block = OriginalLodProvider::BLOCK_CELLS;
    if (cell >= 0) {
        return (cell / block) * block;
    }
    return -(((-cell) + block - 1) / block) * block;
}

}  // namespace

OriginalLodProvider& OriginalLodProvider::instance() {
    static OriginalLodProvider provider;
    return provider;
}

uint64_t OriginalLodProvider::blockKey(int32_t blockX, int32_t blockY) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(blockX)) << 32) |
           static_cast<uint64_t>(static_cast<uint32_t>(blockY));
}

int32_t OriginalLodProvider::blockKeyX(uint64_t key) {
    return static_cast<int32_t>(static_cast<uint32_t>(key >> 32));
}

int32_t OriginalLodProvider::blockKeyY(uint64_t key) {
    return static_cast<int32_t>(static_cast<uint32_t>(key & 0xFFFFFFFFull));
}

const OriginalLodProvider::WorldspaceIndex* OriginalLodProvider::getOrBuildIndex(
    AssetManager* assetManager, uint32_t worldspaceFormID) {
    if (!assetManager || worldspaceFormID == 0) {
        return nullptr;
    }

    auto it = indices_.find(worldspaceFormID);
    if (it != indices_.end() && it->second.built) {
        return &it->second;
    }

    // The archives are opened during startup. Building the index before that
    // would cache an empty result for the rest of the session.
    if (assetManager->getArchiveCount() == 0) {
        LOGW_DLOD("Original LOD index for worldspace %u deferred: no archives",
                  worldspaceFormID);
        return nullptr;
    }

    WorldspaceIndex& index = indices_[worldspaceFormID];
    index.built = true;

    for (const std::string& path : assetManager->findFiles(kMeshPrefix)) {
        int32_t blockX = 0;
        int32_t blockY = 0;
        if (!parseBlockName(path, worldspaceFormID, blockX, blockY)) {
            continue;
        }
        index.blocks[blockKey(blockX, blockY)].mesh = path;
    }

    for (const std::string& path : assetManager->findFiles(kTexturePrefix)) {
        // "_fn" is the normal map of the same block; the colour map alone is
        // used, so the memory cost stays at 4 MB per block plus mipmaps.
        const std::string stem = textureStem(path);
        if (stem.size() >= 3 && stem.compare(stem.size() - 3, 3, "_fn") == 0) {
            continue;
        }

        int32_t blockX = 0;
        int32_t blockY = 0;
        if (!parseBlockName(path, worldspaceFormID, blockX, blockY)) {
            continue;
        }
        index.blocks[blockKey(blockX, blockY)].texture = path;
    }

    size_t texturedBlocks = 0;
    for (const auto& pair : index.blocks) {
        if (!pair.second.texture.empty()) {
            ++texturedBlocks;
        }
    }
    LOGI_DLOD("Original LOD index for worldspace %u: %zu blocks, %zu textured",
              worldspaceFormID, index.blocks.size(), texturedBlocks);

    return &index;
}

bool OriginalLodProvider::isWorldspaceSupported(AssetManager* assetManager,
                                                uint32_t worldspaceFormID) {
    const WorldspaceIndex* index = getOrBuildIndex(assetManager, worldspaceFormID);
    return index != nullptr && !index->blocks.empty();
}

bool OriginalLodProvider::registerBlock(AssetManager* assetManager,
                                        uint32_t worldspaceFormID,
                                        const BlockPaths& paths,
                                        int32_t blockX, int32_t blockY) {
    std::vector<uint8_t> fileData = assetManager->loadFileData(paths.mesh);
    if (fileData.empty()) {
        LOGW_DLOD("Original LOD block missing from the archives: %s",
                  paths.mesh.c_str());
        return false;
    }

    // The parser is a single shared instance and is reset by every parse, so
    // the geometry has to be copied out before returning.
    std::shared_ptr<NIFParser> parser = assetManager->getNifParser();
    if (!parser || !parser->parseFromData(fileData)) {
        LOGW_DLOD("Original LOD block failed to parse: %s", paths.mesh.c_str());
        return false;
    }

    const std::vector<NIFGeometry> geometries = parser->extractAllGeometry();
    if (geometries.empty()) {
        LOGW_DLOD("Original LOD block held no geometry: %s", paths.mesh.c_str());
        return false;
    }

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (const NIFGeometry& geometry : geometries) {
        totalVertices += geometry.vertices.size();
        totalIndices += geometry.triangles.size() * 3;
    }
    // The LOD shader path uses 16 bit indices.
    if (totalVertices == 0 || totalVertices > 65535 || totalIndices == 0) {
        LOGW_DLOD("Original LOD block has an unsupported vertex count (%zu): %s",
                  totalVertices, paths.mesh.c_str());
        return false;
    }

    LodMeshData mesh;
    mesh.blockLod = true;
    mesh.lodLevel = 0;
    mesh.cellX = blockX;
    mesh.cellY = blockY;
    mesh.vertices.reserve(totalVertices * 8);
    mesh.indices.reserve(totalIndices);

    // The shipped blocks are already in block local space: X and Y sweep
    // 0..131072 (one block) and Z is the height, so the transform of the parsed
    // node is identity and only the block origin and the Z-up to Y-up swap are
    // needed. The UVs match the colour map as stored, without a V flip.
    const float blockOriginX = static_cast<float>(blockX) * static_cast<float>(CELL_SIZE);
    const float blockOriginZ = static_cast<float>(blockY) * static_cast<float>(CELL_SIZE);

    for (const NIFGeometry& geometry : geometries) {
        const size_t vertexCount = geometry.vertices.size();
        const uint16_t baseIndex = static_cast<uint16_t>(mesh.vertices.size() / 8);

        for (size_t i = 0; i < vertexCount; ++i) {
            const NIFVector3& position = geometry.vertices[i];
            mesh.vertices.push_back(blockOriginX + position.x);
            mesh.vertices.push_back(position.z);
            mesh.vertices.push_back(blockOriginZ + position.y);

            const glm::vec2 uv = (i < geometry.texCoords.size())
                                     ? geometry.texCoords[i]
                                     : glm::vec2(0.0f, 0.0f);
            mesh.vertices.push_back(uv.x);
            mesh.vertices.push_back(uv.y);

            mesh.vertices.push_back(1.0f);
            mesh.vertices.push_back(1.0f);
            mesh.vertices.push_back(1.0f);
        }

        for (const NIFTriangle& triangle : geometry.triangles) {
            if (triangle.v0 >= vertexCount || triangle.v1 >= vertexCount ||
                triangle.v2 >= vertexCount) {
                continue;
            }
            mesh.indices.push_back(static_cast<uint16_t>(baseIndex + triangle.v0));
            mesh.indices.push_back(static_cast<uint16_t>(baseIndex + triangle.v1));
            mesh.indices.push_back(static_cast<uint16_t>(baseIndex + triangle.v2));
        }
    }

    if (mesh.indices.empty()) {
        LOGW_DLOD("Original LOD block produced no triangles: %s", paths.mesh.c_str());
        return false;
    }

    std::shared_ptr<Material> texture;
    if (!paths.texture.empty()) {
        texture = assetManager->loadDDSTexture(paths.texture);
        if (texture && texture->hasTexture()) {
            mesh.textureId = texture->getTextureId();
            ++loadedTextures_;
        } else {
            // The block still fills the horizon hole without a colour map, and
            // at this distance fog dominates, so the seam stays hidden. The
            // material is dropped so it is not retried every frame.
            texture.reset();
            ++failedTextures_;
            LOGW_DLOD("Original LOD texture unavailable: %s", paths.texture.c_str());
        }
    }

    const std::string meshId = "origblk_" + std::to_string(worldspaceFormID) + "_" +
                               std::to_string(blockX) + "_" + std::to_string(blockY);
    DistantLodManager::instance().registerLodMesh(meshId, mesh);

    ActiveBlock active;
    active.meshId = meshId;
    active.texture = std::move(texture);
    activeBlocks_[blockKey(blockX, blockY)] = std::move(active);
    ++registeredBlocks_;

    LOGI_DLOD("Original LOD block %d,%d registered (%zu verts, %zu tris, tex=%u)",
              blockX, blockY, totalVertices, totalIndices, mesh.textureId);
    return true;
}

void OriginalLodProvider::unregisterActiveBlocks() {
    if (activeBlocks_.empty()) {
        return;
    }
    DistantLodManager& lod = DistantLodManager::instance();
    if (lod.isInitialized()) {
        for (const auto& pair : activeBlocks_) {
            lod.unregisterLodMesh(pair.second.meshId);
        }
    }
    activeBlocks_.clear();
}

int OriginalLodProvider::update(AssetManager* assetManager,
                                uint32_t worldspaceFormID,
                                const glm::vec3& playerPos,
                                float coverageDistance) {
    DistantLodManager& lod = DistantLodManager::instance();
    if (!lod.isInitialized()) {
        activeBlocks_.clear();
        return 0;
    }

    if (worldspaceFormID != currentWorldspace_) {
        unregisterActiveBlocks();
        failedBlocks_.clear();
        currentWorldspace_ = worldspaceFormID;
    }

    const WorldspaceIndex* index = getOrBuildIndex(assetManager, worldspaceFormID);
    if (!index || index->blocks.empty()) {
        unregisterActiveBlocks();
        return 0;
    }

    // Blocks overlapping the coverage box. The player sits inside one block, so
    // this resolves to at most 2x2 blocks for a coverage of 20000 units.
    const int32_t minBlockX = floorToBlock(static_cast<int32_t>(
        std::floor((playerPos.x - coverageDistance) / static_cast<float>(CELL_SIZE))));
    const int32_t maxBlockX = floorToBlock(static_cast<int32_t>(
        std::floor((playerPos.x + coverageDistance) / static_cast<float>(CELL_SIZE))));
    const int32_t minBlockY = floorToBlock(static_cast<int32_t>(
        std::floor((playerPos.z - coverageDistance) / static_cast<float>(CELL_SIZE))));
    const int32_t maxBlockY = floorToBlock(static_cast<int32_t>(
        std::floor((playerPos.z + coverageDistance) / static_cast<float>(CELL_SIZE))));

    std::unordered_set<uint64_t> needed;
    for (int32_t blockY = minBlockY; blockY <= maxBlockY; blockY += BLOCK_CELLS) {
        for (int32_t blockX = minBlockX; blockX <= maxBlockX; blockX += BLOCK_CELLS) {
            const uint64_t key = blockKey(blockX, blockY);
            if (index->blocks.find(key) != index->blocks.end()) {
                needed.insert(key);
            }
        }
    }

    for (auto it = activeBlocks_.begin(); it != activeBlocks_.end();) {
        if (needed.find(it->first) == needed.end()) {
            lod.unregisterLodMesh(it->second.meshId);
            it = activeBlocks_.erase(it);
        } else {
            ++it;
        }
    }

    for (uint64_t key : needed) {
        if (activeBlocks_.find(key) != activeBlocks_.end()) {
            continue;
        }
        if (failedBlocks_.find(key) != failedBlocks_.end()) {
            continue;
        }
        const auto pathIt = index->blocks.find(key);
        if (pathIt == index->blocks.end()) {
            continue;
        }
        if (!registerBlock(assetManager, worldspaceFormID, pathIt->second,
                           blockKeyX(key), blockKeyY(key))) {
            failedBlocks_.insert(key);
        }
    }

    return static_cast<int>(activeBlocks_.size());
}

void OriginalLodProvider::reset() {
    unregisterActiveBlocks();
    failedBlocks_.clear();
    indices_.clear();
    currentWorldspace_ = 0;
    registeredBlocks_ = 0;
    loadedTextures_ = 0;
    failedTextures_ = 0;
    LOGI_DLOD("Original LOD provider reset");
}
