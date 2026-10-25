#include "lod_block_tests.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>

#include "../assets/nif_parser.h"
#include "../world/distant_lod/lod_block_rules.h"

namespace {

// Tamriel: the worldspace that ships the pre-baked landscape LOD.
constexpr uint32_t kTamrielFormID = 60;

constexpr size_t kDefaultBlockLimit = 100;

// The shipped blocks keep their UVs in [0, 1]; a small slack keeps float
// rounding in the NIF reader from failing a block.
constexpr float kUvTolerance = 0.001f;

// Height band of a block. The packing moves the NIF Z axis into Y, so a block
// whose axes were swapped lands far outside this - the horizontal extent alone
// can reach 131072.
constexpr float kMinHeight = -4096.0f;
constexpr float kMaxHeight = 65536.0f;

float nowMs() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<float, std::milli>(clock::now().time_since_epoch())
        .count();
}

bool hasNifExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".nif";
}

void collectNifFiles(const std::filesystem::path& directory, bool recursive,
                     std::vector<std::string>& out) {
    if (!std::filesystem::is_directory(directory)) {
        return;
    }
    std::error_code ec;
    if (recursive) {
        for (std::filesystem::recursive_directory_iterator it(directory, ec), end;
             !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec) || !hasNifExtension(it->path())) {
                continue;
            }
            out.push_back(it->path().string());
        }
    } else {
        for (std::filesystem::directory_iterator it(directory, ec), end;
             !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec) || !hasNifExtension(it->path())) {
                continue;
            }
            out.push_back(it->path().string());
        }
    }
}

}  // namespace

void LodBlockTests::record(const std::string& name, bool passed, const std::string& msg,
                           float ms) {
    results.push_back({name, passed, msg, ms});
    std::cout << "[LodBlockTests] " << (passed ? "PASS" : "FAIL") << " " << name;
    if (!msg.empty()) {
        std::cout << " - " << msg;
    }
    std::cout << " (" << ms << " ms)" << std::endl;
}

int LodBlockTests::getPassCount() const {
    int count = 0;
    for (const LodBlockTestResult& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int LodBlockTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string LodBlockTests::getSummary() const {
    std::ostringstream out;
    out << "LOD Block Test Results\n";
    out << "Total: " << results.size() << " | Pass: " << getPassCount()
        << " | Fail: " << getFailCount();
    return out.str();
}

// ---------------------------------------------------------------------------
// Tests that need no assets
// ---------------------------------------------------------------------------

void LodBlockTests::testNameGrammar() {
    const float start = nowMs();
    size_t mismatches = 0;
    std::string firstFailure;

    struct NameCase {
        const char* name;
        bool accepted;
        int32_t blockX;
        int32_t blockY;
    };
    const NameCase cases[] = {
        {"60.00.00.32.nif", true, 0, 0},
        {"60.-32.64.32.nif", true, -32, 64},
        {"60.32.-64.32.nif", true, 32, -64},
        // The provenance prefix digits are not rewritten, so "0" also parses.
        {"60.0.0.32.nif", true, 0, 0},
        {"meshes\\landscape\\lod\\60.-32.64.32.nif", true, -32, 64},
        {"textures\\landscapelod\\generated\\60.00.00.32.dds", true, 0, 0},
        // Not the 32 cell block size the game ships.
        {"60.00.00.16.nif", false, 0, 0},
        // Another worldspace.
        {"61.00.00.32.nif", false, 0, 0},
        // A zero form ID never names a worldspace.
        {"0.00.00.32.nif", false, 0, 0},
        // Strict integer parsing: "60x" is not 60 and ".." is not a number.
        {"60x.00.00.32.nif", false, 0, 0},
        {"60..00.32.nif", false, 0, 0},
        {"60.00.00.x.32.nif", false, 0, 0},
        // Too few tokens, and a backup suffix is not the extension field.
        {"60.00.00.32", false, 0, 0},
        {"60.00.00.32.nif.bak", false, 0, 0},
        {"", false, 0, 0},
    };

    for (const NameCase& testCase : cases) {
        int32_t blockX = 0;
        int32_t blockY = 0;
        const bool parsed =
            lod_block::parseBlockName(testCase.name, kTamrielFormID, blockX, blockY);
        if (parsed != testCase.accepted ||
            (parsed && (blockX != testCase.blockX || blockY != testCase.blockY))) {
            ++mismatches;
            if (firstFailure.empty()) {
                firstFailure = std::string("\"") + testCase.name + "\"";
            }
        }
    }

    std::ostringstream message;
    if (mismatches == 0) {
        message << (sizeof(cases) / sizeof(cases[0])) << " names classified as expected";
    } else {
        message << mismatches << " names misclassified, first " << firstFailure;
    }
    record("BlockNameGrammar", mismatches == 0, message.str(), nowMs() - start);
}

void LodBlockTests::testPackingArithmetic() {
    const float start = nowMs();

    NIFGeometry first;
    first.vertices.push_back({1.0f, 2.0f, 3.0f});
    first.vertices.push_back({4.0f, 5.0f, 6.0f});
    first.vertices.push_back({7.0f, 8.0f, 9.0f});
    first.texCoords.push_back(glm::vec2(0.25f, 0.5f));
    // The second vertex has no UV and must fall back to (0, 0).
    first.triangles.push_back({0, 1, 2});

    NIFGeometry second;
    second.vertices.push_back({10.0f, 11.0f, 12.0f});
    second.texCoords.push_back(glm::vec2(0.75f, 1.0f));
    second.triangles.push_back({0, 0, 0});

    const std::vector<NIFGeometry> geometries = {first, second};
    lod_block::BlockMeshData packed;
    const lod_block::BlockMeshStatus status =
        lod_block::buildBlockMesh(geometries, 2, -3, packed);

    std::string failure;
    if (status != lod_block::BlockMeshStatus::Ok) {
        failure = "status is not Ok";
    } else if (packed.vertexCount != 4 || packed.triangleCount != 2 ||
               packed.skippedTriangles != 0) {
        failure = "counters are wrong";
    } else if (packed.vertices.size() != 4 * lod_block::kFloatsPerVertex) {
        failure = "vertex stride is wrong";
    } else if (packed.indices.size() != 6) {
        failure = "index count is wrong";
    } else {
        // Block (2, -3) has its origin at (8192, -12288).
        const float* v0 = &packed.vertices[0];
        const float* v1 = &packed.vertices[lod_block::kFloatsPerVertex];
        const float* v3 = &packed.vertices[3 * lod_block::kFloatsPerVertex];
        if (std::fabs(v0[0] - 8193.0f) > 0.001f || std::fabs(v0[1] - 3.0f) > 0.001f ||
            std::fabs(v0[2] - (-12286.0f)) > 0.001f) {
            failure = "vertex 0 placement is wrong";
        } else if (std::fabs(v0[3] - 0.25f) > 0.001f ||
                   std::fabs(v0[4] - 0.5f) > 0.001f) {
            failure = "vertex 0 UV is wrong";
        } else if (v0[5] != 1.0f || v0[6] != 1.0f || v0[7] != 1.0f) {
            failure = "vertex 0 colour is wrong";
        } else if (v1[3] != 0.0f || v1[4] != 0.0f) {
            failure = "missing UV did not fall back to (0, 0)";
        } else if (std::fabs(v3[0] - 8202.0f) > 0.001f ||
                   std::fabs(v3[2] - (-12277.0f)) > 0.001f) {
            failure = "second geometry was not placed";
        } else if (packed.indices[0] != 0 || packed.indices[1] != 1 ||
                   packed.indices[2] != 2) {
            failure = "first geometry indices are wrong";
        } else if (packed.indices[3] != 3 || packed.indices[4] != 3 ||
                   packed.indices[5] != 3) {
            failure = "second geometry indices were not rebased";
        }
    }

    record("BlockVertexPacking", failure.empty(), failure, nowMs() - start);
}

void LodBlockTests::testPackingRejectsUnusableGeometry() {
    const float start = nowMs();
    std::string failure;

    NIFGeometry triangleOnly;
    triangleOnly.vertices.push_back({0.0f, 0.0f, 0.0f});
    triangleOnly.vertices.push_back({1.0f, 0.0f, 0.0f});
    triangleOnly.vertices.push_back({0.0f, 1.0f, 0.0f});
    triangleOnly.triangles.push_back({0, 1, 2});

    lod_block::BlockMeshData packed;

    // A block that held no geometry at all.
    if (lod_block::buildBlockMesh({}, 0, 0, packed) !=
        lod_block::BlockMeshStatus::EmptyGeometry) {
        failure = "an empty block was not rejected";
    } else if (!packed.vertices.empty() || !packed.indices.empty()) {
        failure = "a rejected block kept its buffers";
    }

    // Geometry without a single usable triangle.
    if (failure.empty()) {
        NIFGeometry noTriangles;
        noTriangles.vertices = triangleOnly.vertices;
        if (lod_block::buildBlockMesh({noTriangles}, 0, 0, packed) !=
            lod_block::BlockMeshStatus::NoUsableTriangles) {
            failure = "geometry without triangles was not rejected";
        }
    }

    // One vertex more than a 16 bit index can address.
    if (failure.empty()) {
        NIFGeometry overfull;
        overfull.vertices.resize(lod_block::kMaxVertices + 1);
        overfull.triangles.push_back({0, 1, 2});
        const lod_block::BlockMeshStatus status =
            lod_block::buildBlockMesh({overfull}, 0, 0, packed);
        if (status != lod_block::BlockMeshStatus::VertexCountUnsupported) {
            failure = "an overfull block was not rejected";
        } else if (packed.vertexCount != lod_block::kMaxVertices + 1) {
            failure = "an overfull block did not report its vertex count";
        }
    }

    // Exactly the addressable maximum is accepted.
    if (failure.empty()) {
        NIFGeometry atLimit;
        atLimit.vertices.resize(lod_block::kMaxVertices);
        atLimit.triangles.push_back({0, 1, 2});
        if (lod_block::buildBlockMesh({atLimit}, 0, 0, packed) !=
            lod_block::BlockMeshStatus::Ok) {
            failure = "the maximum vertex count was rejected";
        }
    }

    // A triangle pointing past its own geometry is dropped, not wrapped onto a
    // neighbouring geometry.
    if (failure.empty()) {
        NIFGeometry stray = triangleOnly;
        stray.triangles.push_back({0, 1, 3});
        const lod_block::BlockMeshStatus status =
            lod_block::buildBlockMesh({stray}, 0, 0, packed);
        if (status != lod_block::BlockMeshStatus::Ok) {
            failure = "a block with one stray triangle was rejected";
        } else if (packed.skippedTriangles != 1 || packed.triangleCount != 1) {
            failure = "the stray triangle was not counted as skipped";
        } else if (packed.indices.size() != 3) {
            failure = "the stray triangle was kept";
        }
    }

    record("BlockPackingRejections", failure.empty(), failure, nowMs() - start);
}

// ---------------------------------------------------------------------------
// Tests over the shipped blocks
// ---------------------------------------------------------------------------

void LodBlockTests::testShippedBlocks(const std::vector<BlockSample>& samples) {
    const float start = nowMs();

    size_t parsedBlocks = 0;
    size_t unusableBlocks = 0;
    size_t badVertexCount = 0;
    size_t badIndices = 0;
    size_t badUvPerVertex = 0;
    size_t badUvRange = 0;
    size_t badPlacement = 0;
    size_t badAttributes = 0;
    size_t totalVertices = 0;
    size_t totalTriangles = 0;
    size_t totalSkipped = 0;
    float worstOvershoot = 0.0f;
    float lowestHeight = std::numeric_limits<float>::max();
    float highestHeight = std::numeric_limits<float>::lowest();
    std::string firstFailure;

    const auto noteFailure = [&firstFailure](const std::string& detail) {
        if (firstFailure.empty()) {
            firstFailure = detail;
        }
    };

    for (const BlockSample& sample : samples) {
        const std::string& name = sample.path.substr(
            sample.path.find_last_of("\\/") == std::string::npos
                ? 0
                : sample.path.find_last_of("\\/") + 1);

        NIFParser parser;
        if (!parser.parseFile(sample.path)) {
            ++unusableBlocks;
            noteFailure(name + ": parse failed");
            continue;
        }

        const std::vector<NIFGeometry> geometries = parser.extractAllGeometry();
        if (geometries.empty()) {
            ++unusableBlocks;
            noteFailure(name + ": no geometry");
            continue;
        }

        for (const NIFGeometry& geometry : geometries) {
            if (geometry.texCoords.size() != geometry.vertices.size()) {
                ++badUvPerVertex;
                noteFailure(name + ": " + std::to_string(geometry.vertices.size()) +
                            " vertices but " + std::to_string(geometry.texCoords.size()) +
                            " UVs");
                break;
            }
        }

        lod_block::BlockMeshData packed;
        const lod_block::BlockMeshStatus status =
            lod_block::buildBlockMesh(geometries, sample.blockX, sample.blockY, packed);
        if (status != lod_block::BlockMeshStatus::Ok) {
            ++unusableBlocks;
            noteFailure(name + ": packing status " +
                        std::to_string(static_cast<int>(status)));
            continue;
        }
        ++parsedBlocks;

        const size_t vertexCount = packed.vertices.size() / lod_block::kFloatsPerVertex;
        totalVertices += vertexCount;
        totalTriangles += packed.triangleCount;
        totalSkipped += packed.skippedTriangles;

        if (packed.vertices.size() % lod_block::kFloatsPerVertex != 0 || vertexCount == 0 ||
            vertexCount > lod_block::kMaxVertices || vertexCount != packed.vertexCount) {
            ++badVertexCount;
            noteFailure(name + ": vertex count " + std::to_string(vertexCount));
        }

        size_t highestIndex = 0;
        for (uint16_t index : packed.indices) {
            if (index > highestIndex) {
                highestIndex = index;
            }
        }
        if (packed.indices.empty() || packed.indices.size() != packed.triangleCount * 3 ||
            highestIndex >= vertexCount) {
            ++badIndices;
            noteFailure(name + ": index " + std::to_string(highestIndex) + " of " +
                        std::to_string(vertexCount) + " vertices");
        }

        const float blockOriginX = static_cast<float>(sample.blockX) * lod_block::kCellSize;
        const float blockOriginZ = static_cast<float>(sample.blockY) * lod_block::kCellSize;

        bool uvOutOfRange = false;
        bool attributesWrong = false;
        bool placementWrong = false;
        for (size_t i = 0; i < vertexCount; ++i) {
            const float* vertex = &packed.vertices[i * lod_block::kFloatsPerVertex];
            if (vertex[3] < -kUvTolerance || vertex[3] > 1.0f + kUvTolerance ||
                vertex[4] < -kUvTolerance || vertex[4] > 1.0f + kUvTolerance) {
                uvOutOfRange = true;
            }
            if (vertex[5] != 1.0f || vertex[6] != 1.0f || vertex[7] != 1.0f) {
                attributesWrong = true;
            }

            // The block must stay in its own 131072 unit box in the ground
            // plane, and its height must stay inside the band of a landscape.
            const float height = vertex[1];
            if (height < kMinHeight || height > kMaxHeight) {
                placementWrong = true;
            }
            if (height < lowestHeight) {
                lowestHeight = height;
            }
            if (height > highestHeight) {
                highestHeight = height;
            }

            const float overshoots[] = {
                blockOriginX - vertex[0],
                vertex[0] - (blockOriginX + lod_block::kBlockSpan),
                blockOriginZ - vertex[2],
                vertex[2] - (blockOriginZ + lod_block::kBlockSpan),
            };
            for (float overshoot : overshoots) {
                if (overshoot > worstOvershoot) {
                    worstOvershoot = overshoot;
                }
                if (overshoot > lod_block::kMaxOvershoot) {
                    placementWrong = true;
                }
            }
        }
        if (uvOutOfRange) {
            ++badUvRange;
            noteFailure(name + ": UV outside [0, 1]");
        }
        if (attributesWrong) {
            ++badAttributes;
            noteFailure(name + ": vertex colour is not 1,1,1");
        }
        if (placementWrong) {
            ++badPlacement;
            noteFailure(name + ": geometry outside its block box");
        }
    }

    const float elapsed = nowMs() - start;
    std::ostringstream counts;
    counts << samples.size() << " blocks checked, " << parsedBlocks << " packed, "
           << totalVertices << " vertices, " << totalTriangles << " triangles"
           << (totalSkipped > 0 ? ", " + std::to_string(totalSkipped) + " skipped" : "");

    record("BlockGeometryPresent", unusableBlocks == 0,
           unusableBlocks == 0
               ? counts.str()
               : counts.str() + "; " + std::to_string(unusableBlocks) +
                     " unusable (" + firstFailure + ")",
           elapsed);

    record("VertexCountInRange", badVertexCount == 0 && parsedBlocks > 0,
           badVertexCount == 0 ? std::to_string(totalVertices) + " vertices in 1.." +
                                     std::to_string(lod_block::kMaxVertices)
                               : std::to_string(badVertexCount) + " blocks out of range",
           0.0f);

    record("TriangleIndicesUsable", badIndices == 0 && parsedBlocks > 0,
           badIndices == 0 ? std::to_string(totalTriangles) + " triangles addressed"
                           : std::to_string(badIndices) + " blocks with bad indices",
           0.0f);

    record("UvPerVertex", badUvPerVertex == 0 && parsedBlocks > 0,
           badUvPerVertex == 0 ? "every vertex carries a UV"
                               : std::to_string(badUvPerVertex) + " blocks fell back to (0, 0)",
           0.0f);

    record("UvRange", badUvRange == 0 && parsedBlocks > 0,
           badUvRange == 0 ? "UVs inside [0, 1]" : std::to_string(badUvRange) + " blocks outside",
           0.0f);

    std::ostringstream placement;
    placement << "worst overshoot " << worstOvershoot << ", height " << lowestHeight << ".."
              << highestHeight;
    record("BlockWorldPlacement", badPlacement == 0 && parsedBlocks > 0,
           badPlacement == 0 ? placement.str()
                             : placement.str() + "; " + std::to_string(badPlacement) +
                                   " blocks misplaced (" + firstFailure + ")",
           0.0f);

    record("VertexAttributesPacked", badAttributes == 0 && parsedBlocks > 0,
           badAttributes == 0 ? "colours are 1, 1, 1"
                              : std::to_string(badAttributes) + " blocks with wrong colours",
           0.0f);
}

// ---------------------------------------------------------------------------
// Asset discovery
// ---------------------------------------------------------------------------

size_t LodBlockTests::blockLimit() const {
    const char* configured = std::getenv("OBLIVION_LOD_BLOCK_LIMIT");
    if (configured != nullptr) {
        const int parsed = std::atoi(configured);
        if (parsed > 0) {
            return static_cast<size_t>(parsed);
        }
    }
    return kDefaultBlockLimit;
}

std::vector<std::string> LodBlockTests::collectBlockFiles(
    const std::string& assetBasePath) const {
    std::vector<std::string> files;

    // A flat directory of extracted meshes\landscape\lod\*.nif, if one is given.
    const char* lodDirectory = std::getenv("OBLIVION_LOD_NIF_DIR");
    if (lodDirectory != nullptr && lodDirectory[0] != '\0') {
        collectNifFiles(std::filesystem::path(lodDirectory), false, files);
    }

    if (files.empty() && !assetBasePath.empty()) {
        const std::filesystem::path base(assetBasePath);
        collectNifFiles(base / "meshes" / "landscape" / "lod", true, files);
        if (files.empty()) {
            collectNifFiles(base, false, files);
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

std::vector<LodBlockTests::BlockSample> LodBlockTests::sampleBlocks(
    const std::vector<std::string>& files, uint32_t worldspaceFormID) const {
    std::vector<BlockSample> blocks;
    blocks.reserve(files.size());
    for (const std::string& file : files) {
        int32_t blockX = 0;
        int32_t blockY = 0;
        if (!lod_block::parseBlockName(file, worldspaceFormID, blockX, blockY)) {
            continue;
        }
        blocks.push_back({file, blockX, blockY});
    }

    // The files are visited in a stable order so the run covers the same
    // blocks every time, then spread over the whole landscape instead of
    // taking the first N.
    std::sort(blocks.begin(), blocks.end(),
              [](const BlockSample& a, const BlockSample& b) {
                  if (a.blockY != b.blockY) {
                      return a.blockY < b.blockY;
                  }
                  return a.blockX < b.blockX;
              });

    const size_t limit = blockLimit();
    if (blocks.size() > limit) {
        std::vector<BlockSample> sampled;
        sampled.reserve(limit);
        for (size_t i = 0; i < limit; ++i) {
            sampled.push_back(blocks[(i * blocks.size()) / limit]);
        }
        return sampled;
    }
    return blocks;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

bool LodBlockTests::runAllTests(const std::string& assetBasePath) {
    results.clear();

    testNameGrammar();
    testPackingArithmetic();
    testPackingRejectsUnusableGeometry();

    const std::vector<std::string> files = collectBlockFiles(assetBasePath);
    const std::vector<BlockSample> samples = sampleBlocks(files, kTamrielFormID);

    std::cout << "[LodBlockTests] " << files.size() << " landscape LOD files found, "
              << samples.size() << " sampled (limit " << blockLimit() << ")" << std::endl;

    if (samples.empty()) {
        record("SKIP_Assets_Unavailable", true,
               "SKIPPED: assets not available - extracted meshes\\landscape\\lod NIF files "
               "are required (set OBLIVION_LOD_NIF_DIR or OBLIVION_ASSET_BASE)",
               0.0f);
    } else {
        testShippedBlocks(samples);
    }

    return getFailCount() == 0;
}
