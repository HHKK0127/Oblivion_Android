#pragma once

// Phase 65: the rules of the shipped landscape LOD blocks.
//
// The retail game ships its distant terrain pre-baked as whole-landscape
// blocks:
//
//   meshes\landscape\lod\<formID>.<blockX>.<blockY>.32.nif
//   textures\landscapelod\generated\<formID>.<blockX>.<blockY>.32.dds
//
// Each block covers 32 x 32 cells (131072 x 131072 world units) and stores its
// geometry in block local space: X and Y span 0..131072 and Z is the height
// axis. original_lod_provider.cpp used to keep the naming grammar and the
// vertex packing in its own anonymous namespace, so they could only be
// exercised by streaming a real archive. They live here instead: the provider
// and tests/lod_block_tests.cpp include the same code, so the invariants the
// 16 bit GPU upload depends on are covered by the host test suite and cannot
// drift away from the production path.
//
// The constants mirror what the game data actually ships; they are not
// tunables.

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../../assets/nif_types.h"

namespace lod_block {

// One block is 32 x 32 cells of 4096 world units.
constexpr int32_t kBlockCells = 32;
constexpr float kCellSize = 4096.0f;
constexpr float kBlockSpan = static_cast<float>(kBlockCells) * kCellSize;

// x, y, z, u, v, r, g, b - the stride DistantLodManager uploads.
constexpr size_t kFloatsPerVertex = 8;

// The LOD shader path indexes a block with 16 bit indices.
constexpr size_t kMaxVertices = 65535;

// How far a block may reach outside its own 131072 unit box before it stops
// lining up with its neighbours. The shipped blocks stay inside it.
constexpr float kMaxOvershoot = 64.0f;

// Strict integer parse: the whole token has to be numeric, so a stray file such
// as "60x.00.00.32.nif" is rejected instead of silently parsed as 60.
inline bool parseStrictInteger(const std::string& text, long& out) {
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
// "00" (the shipped files are e.g. "60.00.00.32.nif", "60.-32.64.32.nif"). The
// extension is not inspected, so the colour map of a block parses exactly like
// its mesh.
inline bool parseBlockName(const std::string& path, uint32_t worldspaceFormID,
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
    if (!parseStrictInteger(parts[0], formID) ||
        !parseStrictInteger(parts[1], blockX) ||
        !parseStrictInteger(parts[2], blockY) ||
        !parseStrictInteger(parts[3], blockCells)) {
        return false;
    }
    if (formID <= 0 || static_cast<uint32_t>(formID) != worldspaceFormID) {
        return false;
    }
    // Only the 32 cell block size shipped by the game is handled.
    if (blockCells != kBlockCells) {
        return false;
    }

    outBlockX = static_cast<int32_t>(blockX);
    outBlockY = static_cast<int32_t>(blockY);
    return true;
}

// Rounds a cell coordinate down to the block that contains it. C++ integer
// division truncates towards zero, which would be wrong for negative cells.
inline int32_t floorToBlock(int32_t cell) {
    if (cell >= 0) {
        return (cell / kBlockCells) * kBlockCells;
    }
    return -(((-cell) + kBlockCells - 1) / kBlockCells) * kBlockCells;
}

enum class BlockMeshStatus {
    Ok,
    EmptyGeometry,
    VertexCountUnsupported,
    NoUsableTriangles,
};

struct BlockMeshData {
    // Packed vertices, kFloatsPerVertex floats each.
    std::vector<float> vertices;
    std::vector<uint16_t> indices;
    // Vertices in the source geometries.
    size_t vertexCount = 0;
    // Triangles kept in indices.
    size_t triangleCount = 0;
    // Triangles dropped because they pointed past the end of their geometry.
    size_t skippedTriangles = 0;
};

// Packs the geometries of one LOD block into the flat vertex array
// DistantLodManager uploads, rebasing every geometry's indices onto it.
//
// Triangles that point past the end of their own geometry are skipped rather
// than rejected: the shipped blocks are consistent, but such a triangle must
// never fabricate an index that reads another geometry's vertices.
inline BlockMeshStatus buildBlockMesh(const std::vector<NIFGeometry>& geometries,
                                      int32_t blockX, int32_t blockY,
                                      BlockMeshData& out) {
    out = BlockMeshData();

    if (geometries.empty()) {
        return BlockMeshStatus::EmptyGeometry;
    }

    size_t sourceVertices = 0;
    size_t sourceTriangles = 0;
    for (const NIFGeometry& geometry : geometries) {
        sourceVertices += geometry.vertices.size();
        sourceTriangles += geometry.triangles.size();
    }
    out.vertexCount = sourceVertices;
    // The LOD shader path uses 16 bit indices.
    if (sourceVertices == 0 || sourceVertices > kMaxVertices) {
        return BlockMeshStatus::VertexCountUnsupported;
    }

    out.vertices.reserve(sourceVertices * kFloatsPerVertex);
    out.indices.reserve(sourceTriangles * 3);

    // The shipped blocks are already in block local space: X and Y sweep
    // 0..131072 (one block) and Z is the height, so the transform of the parsed
    // node is identity and only the block origin and the Z-up to Y-up swap are
    // needed. The UVs match the colour map as stored, without a V flip.
    const float blockOriginX = static_cast<float>(blockX) * kCellSize;
    const float blockOriginZ = static_cast<float>(blockY) * kCellSize;

    for (const NIFGeometry& geometry : geometries) {
        const size_t vertexCount = geometry.vertices.size();
        const uint16_t baseIndex =
            static_cast<uint16_t>(out.vertices.size() / kFloatsPerVertex);

        for (size_t i = 0; i < vertexCount; ++i) {
            const NIFVector3& position = geometry.vertices[i];
            out.vertices.push_back(blockOriginX + position.x);
            out.vertices.push_back(position.z);
            out.vertices.push_back(blockOriginZ + position.y);

            const glm::vec2 uv = (i < geometry.texCoords.size())
                                     ? geometry.texCoords[i]
                                     : glm::vec2(0.0f, 0.0f);
            out.vertices.push_back(uv.x);
            out.vertices.push_back(uv.y);

            out.vertices.push_back(1.0f);
            out.vertices.push_back(1.0f);
            out.vertices.push_back(1.0f);
        }

        for (const NIFTriangle& triangle : geometry.triangles) {
            if (triangle.v0 >= vertexCount || triangle.v1 >= vertexCount ||
                triangle.v2 >= vertexCount) {
                ++out.skippedTriangles;
                continue;
            }
            out.indices.push_back(static_cast<uint16_t>(baseIndex + triangle.v0));
            out.indices.push_back(static_cast<uint16_t>(baseIndex + triangle.v1));
            out.indices.push_back(static_cast<uint16_t>(baseIndex + triangle.v2));
            ++out.triangleCount;
        }
    }

    if (out.indices.empty()) {
        return BlockMeshStatus::NoUsableTriangles;
    }
    return BlockMeshStatus::Ok;
}

}  // namespace lod_block
