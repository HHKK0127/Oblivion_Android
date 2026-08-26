#include "skin_partition_packer.h"
#include <algorithm>
#include <cstring>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#define LOG_TAG "SkinPartitionPacker"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

uint8_t SkinPartitionPacker::popcount(BoneMask mask) {
    uint8_t count = 0;
    while (mask) {
        count += (mask & 1);
        mask >>= 1;
    }
    return count;
}

BoneMask SkinPartitionPacker::buildBoneMask(const std::array<uint16_t, 4>& indices,
                                            uint8_t numBones) {
    BoneMask mask = 0;
    for (uint8_t i = 0; i < numBones; ++i) {
        if (indices[i] < 64) {
            mask |= (1ULL << indices[i]);
        }
    }
    return mask;
}

void SkinPartitionPacker::normalize(PackedVertex& v) {
    float sum = 0.0f;
    for (uint8_t i = 0; i < v.numBones; ++i) sum += v.weights[i];
    if (sum > 0.0001f) {
        for (uint8_t i = 0; i < v.numBones; ++i) {
            v.weights[i] /= sum;
        }
    }
}

PackedVertex SkinPartitionPacker::packVertex(const std::vector<NIFVertexWeight>& sourceWeights,
                                             const PackOptions& options) {
    PackedVertex result;
    // Collect (boneIndex, weight) pairs and filter by threshold
    std::vector<std::pair<uint16_t, float>> filtered;
    for (const auto& w : sourceWeights) {
        if (w.weight >= options.weightThreshold) {
            filtered.emplace_back(w.vertexIndex, w.weight);
        }
    }

    // Sort by weight descending, take top N
    std::sort(filtered.begin(), filtered.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    uint32_t take = std::min<uint32_t>(filtered.size(), options.maxBonesPerVertex);
    result.numBones = static_cast<uint8_t>(take);
    for (uint32_t i = 0; i < take; ++i) {
        result.indices[i] = filtered[i].first;
        result.weights[i] = filtered[i].second;
    }

    if (options.normalizeWeights) {
        normalize(result);
    }

    result.boneMask = buildBoneMask(result.indices, result.numBones);
    return result;
}

int SkinPartitionPacker::findBestPartition(const BoneMask& vertexMask,
                                           uint8_t vertexBoneCount,
                                           const std::vector<PartitionCandidate>& candidates,
                                           const PackOptions& options) {
    int bestIdx = -1;
    uint8_t bestCommon = 0;
    bool bestExact = false;

    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& c = candidates[i];

        // Skip if partition is full
        if (c.vertexIndices.size() >= options.maxVerticesPerPartition) continue;

        // Check bone count limit
        BoneMask merged = c.boneMask | vertexMask;
        uint8_t mergedCount = popcount(merged);
        if (mergedCount > options.maxBonesPerPartition) continue;

        // Count common bones
        uint8_t common = popcount(c.boneMask & vertexMask);
        bool exact = (mergedCount == c.boneCount);

        if (bestIdx == -1 || common > bestCommon || (common == bestCommon && exact && !bestExact)) {
            bestIdx = static_cast<int>(i);
            bestCommon = common;
            bestExact = exact;
        }
    }
    return bestIdx;
}

std::vector<NIFSkinPartition::Partition> SkinPartitionPacker::clusterPartitions(
    const std::vector<PackedVertex>& packedVertices,
    const std::vector<uint16_t>& originalIndices,
    const PackOptions& options) {

    std::vector<PartitionCandidate> candidates;
    std::vector<int> vertexAssignment(packedVertices.size(), -1);

    for (size_t v = 0; v < packedVertices.size(); ++v) {
        const auto& pv = packedVertices[v];
        if (pv.numBones == 0) continue;

        int bestIdx = findBestPartition(pv.boneMask, pv.numBones, candidates, options);

        if (bestIdx >= 0) {
            // Merge into existing partition
            candidates[bestIdx].boneMask |= pv.boneMask;
            candidates[bestIdx].boneCount = popcount(candidates[bestIdx].boneMask);
            candidates[bestIdx].vertexIndices.push_back(static_cast<uint32_t>(v));
            vertexAssignment[v] = bestIdx;
        } else {
            // Create new partition
            PartitionCandidate newCand;
            newCand.boneMask = pv.boneMask;
            newCand.boneCount = pv.numBones;
            newCand.vertexIndices.push_back(static_cast<uint32_t>(v));
            vertexAssignment[v] = static_cast<int>(candidates.size());
            candidates.push_back(newCand);
        }
    }

    // Convert candidates to NIFSkinPartition::Partition
    std::vector<NIFSkinPartition::Partition> partitions;
    for (const auto& cand : candidates) {
        NIFSkinPartition::Partition p;
        // Extract bone indices from mask
        for (int b = 0; b < 64; ++b) {
            if (cand.boneMask & (1ULL << b)) {
                p.bonePalette.bones.push_back(static_cast<uint16_t>(b));
            }
        }
        // Pack weights for each vertex in this partition
        for (uint32_t vIdx : cand.vertexIndices) {
            GPUVertexWeight gvw;
            const auto& pv = packedVertices[vIdx];
            for (int k = 0; k < 4; ++k) {
                gvw.boneIndices[k] = pv.indices[k];
                gvw.weights[k] = pv.weights[k];
            }
            p.packedWeights.push_back(gvw);
            p.indices.push_back(originalIndices[vIdx]);
            p.numVertices++;
        }
        // Triangles will be filled by caller based on indices
        partitions.push_back(p);
    }
    return partitions;
}

bool SkinPartitionPacker::pack(const NIFSkinData& skinData,
                                uint32_t numVertices,
                                const PackOptions& options,
                                NIFSkinPartition& outPartition) {
    outPartition.maxBonesPerPartition = options.maxBonesPerPartition;
    outPartition.maxBonesPerVertex = options.maxBonesPerVertex;

    // Step 1: Pack each vertex
    std::vector<PackedVertex> packedVertices;
    packedVertices.reserve(numVertices);

    if (skinData.boneData.empty()) {
        LOGD("No bone data, empty pack");
        return true;
    }

    // Build per-vertex weight list from NiSkinData
    std::vector<std::vector<NIFVertexWeight>> perVertexWeights(numVertices);
    for (size_t bi = 0; bi < skinData.boneData.size(); ++bi) {
        const auto& bone = skinData.boneData[bi];
        for (const auto& vw : bone.vertexWeights) {
            if (vw.vertexIndex < numVertices) {
                perVertexWeights[vw.vertexIndex].push_back({static_cast<uint16_t>(bi), vw.weight});
            }
        }
    }

    for (uint32_t v = 0; v < numVertices; ++v) {
        packedVertices.push_back(packVertex(perVertexWeights[v], options));
    }

    // Step 2: Build original indices (0..numVertices-1)
    std::vector<uint16_t> originalIndices(numVertices);
    for (uint32_t v = 0; v < numVertices; ++v) {
        originalIndices[v] = static_cast<uint16_t>(v);
    }

    // Step 3: Cluster into partitions
    outPartition.partitions = clusterPartitions(packedVertices, originalIndices, options);

    LOGD("Packed %u vertices into %zu partitions", numVertices, outPartition.partitions.size());
    return true;
}
