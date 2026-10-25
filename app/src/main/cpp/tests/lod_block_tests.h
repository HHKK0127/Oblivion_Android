#pragma once

// Phase 65: invariant tests for the shipped landscape LOD blocks.
//
// The checks reproduce what original_lod_provider.cpp does when it streams a
// block in, but they call the same lod_block_rules.h helpers the provider uses,
// so a change to the naming grammar or the vertex packing fails this suite
// instead of silently breaking the 16 bit upload.
//
// Blocks are searched for in OBLIVION_LOD_NIF_DIR when that is set (a flat
// directory of extracted meshes\landscape\lod\*.nif), otherwise in
// <OBLIVION_ASSET_BASE>/meshes/landscape/lod, and finally directly in
// OBLIVION_ASSET_BASE. OBLIVION_LOD_BLOCK_LIMIT caps how many of them are
// checked (default 100).
//
// Real Oblivion assets are not redistributable, so the suite records a single
// SKIP_Assets_Unavailable entry and passes when no block can be found.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct LodBlockTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class LodBlockTests {
public:
    // Returns true if all tests pass.
    bool runAllTests(const std::string& assetBasePath);

    // Valid after runAllTests().
    const std::vector<LodBlockTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    // One shipped block: its path and the grid coordinates parsed out of it.
    struct BlockSample {
        std::string path;
        int32_t blockX;
        int32_t blockY;
    };

    // Tests that need no assets.
    void testNameGrammar();
    void testPackingArithmetic();
    void testPackingRejectsUnusableGeometry();

    // Tests over the shipped blocks, when any are available.
    void testShippedBlocks(const std::vector<BlockSample>& samples);

    void record(const std::string& name, bool passed, const std::string& msg = "",
                float ms = 0.0f);
    std::vector<std::string> collectBlockFiles(const std::string& assetBasePath) const;
    std::vector<BlockSample> sampleBlocks(const std::vector<std::string>& files,
                                          uint32_t worldspaceFormID) const;
    size_t blockLimit() const;

    std::vector<LodBlockTestResult> results;
};
