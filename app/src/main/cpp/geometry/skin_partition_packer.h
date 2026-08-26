#pragma once

#include "nif_types.h"
#include <vector>
#include <array>
#include <cstdint>
#include <unordered_map>

// Bitmask for bone set tracking (supports up to 64 bones per partition)
using BoneMask = uint64_t;

struct PackOptions {
    uint32_t maxBonesPerVertex = 4;
    uint32_t maxBonesPerPartition = 64;
    uint32_t maxVerticesPerPartition = 4096;
    float weightThreshold = 0.001f;
    bool normalizeWeights = true;
};

struct PackedVertex {
    std::array<uint16_t, 4> indices = {0, 0, 0, 0};
    std::array<float, 4> weights = {0.0f, 0.0f, 0.0f, 0.0f};
    BoneMask boneMask = 0;
    uint8_t numBones = 0;
};

struct PartitionCandidate {
    BoneMask boneMask = 0;
    std::vector<uint32_t> vertexIndices;
    uint8_t boneCount = 0;
};

class SkinPartitionPacker {
public:
    // Main entry: pack variable-length weights into GPU-ready partitions
    static bool pack(const NIFSkinData& skinData,
                     uint32_t numVertices,
                     const PackOptions& options,
                     NIFSkinPartition& outPartition);

    // Pack a single vertex's weights (top N by weight, threshold, normalize)
    static PackedVertex packVertex(const std::vector<NIFVertexWeight>& sourceWeights,
                                   const PackOptions& options);

    // Cluster vertices into partitions by bone usage (greedy with bitmask)
    static std::vector<NIFSkinPartition::Partition> clusterPartitions(
        const std::vector<PackedVertex>& packedVertices,
        const std::vector<uint16_t>& originalIndices,
        const PackOptions& options);

private:
    // Count set bits in bone mask
    static uint8_t popcount(BoneMask mask);

    // Normalize weights to sum to 1.0
    static void normalize(PackedVertex& v);

    // Build BoneMask from vertex weights
    static BoneMask buildBoneMask(const std::array<uint16_t, 4>& indices,
                                  uint8_t numBones);

    // Find best partition to merge into (max common bones)
    static int findBestPartition(const BoneMask& vertexMask,
                                 uint8_t vertexBoneCount,
                                 const std::vector<PartitionCandidate>& candidates,
                                 const PackOptions& options);
};
