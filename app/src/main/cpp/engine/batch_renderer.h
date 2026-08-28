#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <android/log.h>

#define LOG_TAG_BR "BatchRenderer"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_BR(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_BR, __VA_ARGS__)
#else
#define LOGD_BR(...) do {} while(0)
#endif
#define LOGI_BR(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_BR, __VA_ARGS__)
#define LOGW_BR(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_BR, __VA_ARGS__)

// ============================================================================
// Batch Renderer
// Phase 55: Material-based draw call batching for GLES 3.0
// ============================================================================

namespace engine {

// Vertex format for batched rendering
struct BatchVertex {
    float position[3];
    float normal[3];
    float uv[2];
    float color[4];
};

// Draw command
struct DrawCommand {
    uint32_t indexCount;
    uint32_t indexOffset;
    uint32_t vertexOffset;
    uint32_t materialId;
    uint32_t sortOrder; // For transparency sorting
};

// Material state for batching
struct BatchMaterialState {
    uint32_t shaderProgramId;
    uint32_t textureId;
    uint32_t blendMode;      // 0=opaque, 1=alpha, 2=additive
    uint32_t depthWrite;     // 1=enabled, 0=disabled
    uint32_t cullMode;       // 0=none, 1=back, 2=front

    bool operator==(const BatchMaterialState& o) const {
        return shaderProgramId == o.shaderProgramId &&
               textureId == o.textureId &&
               blendMode == o.blendMode &&
               depthWrite == o.depthWrite &&
               cullMode == o.cullMode;
    }
};

struct BatchMaterialStateHash {
    size_t operator()(const BatchMaterialState& s) const {
        size_t h = 0;
        h ^= std::hash<uint32_t>{}(s.shaderProgramId) << 0;
        h ^= std::hash<uint32_t>{}(s.textureId) << 8;
        h ^= std::hash<uint32_t>{}(s.blendMode) << 16;
        h ^= std::hash<uint32_t>{}(s.depthWrite) << 24;
        h ^= std::hash<uint32_t>{}(s.cullMode) << 32;
        return h;
    }
};

// Batch: a group of geometry sharing the same material state
struct RenderBatch {
    BatchMaterialState material;
    std::vector<BatchVertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<DrawCommand> commands;
    uint32_t vboId = 0;
    uint32_t iboId = 0;
    bool uploaded = false;
};

// ============================================================================
// BatchRenderer - material-based draw call batching
// ============================================================================

class BatchRenderer {
public:
    static constexpr size_t MAX_VERTICES_PER_BATCH = 65536;
    static constexpr size_t MAX_INDICES_PER_BATCH = 131072;

    static BatchRenderer& instance() {
        static BatchRenderer inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);
        batches_.clear();
        currentFrameCommands_.clear();
        stats_ = {};
        initialized_ = true;
        LOGI_BR("BatchRenderer initialized");
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        batches_.clear();
        currentFrameCommands_.clear();
        initialized_ = false;
    }

    // --- Submission ---

    // Submit geometry for batching
    uint32_t submit(
        const BatchVertex* vertices, uint32_t vertexCount,
        const uint16_t* indices, uint32_t indexCount,
        const BatchMaterialState& material
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) return 0;

        // Find or create batch for this material
        RenderBatch* batch = findOrCreateBatch(material);
        if (!batch) return 0;

        // Check capacity
        if (batch->vertices.size() + vertexCount > MAX_VERTICES_PER_BATCH ||
            batch->indices.size() + indexCount > MAX_INDICES_PER_BATCH) {
            // Batch full, create new one
            batch = createNewBatch(material);
            if (!batch) return 0;
        }

        // Record draw command
        DrawCommand cmd;
        cmd.indexCount = indexCount;
        cmd.indexOffset = static_cast<uint32_t>(batch->indices.size());
        cmd.vertexOffset = static_cast<uint32_t>(batch->vertices.size());
        cmd.materialId = material.shaderProgramId;
        cmd.sortOrder = (material.blendMode > 0) ? 1 : 0; // Opaque first

        // Append vertices
        batch->vertices.insert(batch->vertices.end(), vertices, vertices + vertexCount);

        // Append indices (offset by current vertex base)
        uint16_t baseVertex = static_cast<uint16_t>(cmd.vertexOffset);
        for (uint32_t i = 0; i < indexCount; i++) {
            batch->indices.push_back(indices[i] + baseVertex);
        }

        batch->commands.push_back(cmd);
        batch->uploaded = false;

        stats_.submittedVertices += vertexCount;
        stats_.submittedIndices += indexCount;
        stats_.submittedCommands++;

        return static_cast<uint32_t>(batch->commands.size() - 1);
    }

    // --- Frame processing ---

    // Sort and optimize batches for rendering
    void beginFrame() {
        std::lock_guard<std::mutex> lock(mutex_);

        stats_.drawCalls = 0;
        stats_.batchedCommands = 0;

        // Sort batches: opaque first, then by shader, then by texture
        std::sort(batches_.begin(), batches_.end(),
            [](const RenderBatch& a, const RenderBatch& b) {
                if (a.material.blendMode != b.material.blendMode)
                    return a.material.blendMode < b.material.blendMode;
                if (a.material.shaderProgramId != b.material.shaderProgramId)
                    return a.material.shaderProgramId < b.material.shaderProgramId;
                return a.material.textureId < b.material.textureId;
            });

        // Sort transparent commands back-to-front
        for (auto& batch : batches_) {
            if (batch.material.blendMode > 0) {
                std::sort(batch.commands.begin(), batch.commands.end(),
                    [](const DrawCommand& a, const DrawCommand& b) {
                        return a.sortOrder > b.sortOrder;
                    });
            }
        }
    }

    // Execute all batched draw calls
    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& batch : batches_) {
            if (batch.commands.empty()) continue;

            // Upload to GPU if needed
            if (!batch.uploaded) {
                uploadBatch(batch);
            }

            // Bind material state
            bindMaterialState(batch.material);

            // Issue draw calls
            for (const auto& cmd : batch.commands) {
                // In real impl: glDrawElements(GL_TRIANGLES, cmd.indexCount,
                //     GL_UNSIGNED_SHORT, cmd.indexOffset * sizeof(uint16_t))
                stats_.drawCalls++;
            }

            stats_.batchedCommands += static_cast<uint32_t>(batch.commands.size());
        }
    }

    void endFrame() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Clear per-frame data but keep GPU resources
        for (auto& batch : batches_) {
            batch.vertices.clear();
            batch.indices.clear();
            batch.commands.clear();
            batch.uploaded = false;
        }

        // Remove empty batches
        batches_.erase(
            std::remove_if(batches_.begin(), batches_.end(),
                [](const RenderBatch& b) { return b.commands.empty(); }),
            batches_.end());
    }

    // --- Statistics ---

    struct BatchStats {
        uint32_t submittedVertices;
        uint32_t submittedIndices;
        uint32_t submittedCommands;
        uint32_t drawCalls;
        uint32_t batchedCommands;
        uint32_t activeBatches;
        float batchingRatio; // batchedCommands / drawCalls
    };

    BatchStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        BatchStats s = stats_;
        s.activeBatches = static_cast<uint32_t>(batches_.size());
        s.batchingRatio = s.drawCalls > 0 ?
            static_cast<float>(s.batchedCommands) / s.drawCalls : 1.0f;
        return s;
    }

    // Clear all batches (e.g., scene change)
    void clearAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        batches_.clear();
        currentFrameCommands_.clear();
    }

private:
    BatchRenderer() = default;

    bool initialized_ = false;
    std::vector<RenderBatch> batches_;
    std::vector<DrawCommand> currentFrameCommands_;
    BatchStats stats_{};

    mutable std::mutex mutex_;

    RenderBatch* findOrCreateBatch(const BatchMaterialState& material) {
        // Find existing batch with same material
        for (auto& batch : batches_) {
            if (batch.material == material &&
                batch.vertices.size() < MAX_VERTICES_PER_BATCH &&
                batch.indices.size() < MAX_INDICES_PER_BATCH) {
                return &batch;
            }
        }
        return createNewBatch(material);
    }

    RenderBatch* createNewBatch(const BatchMaterialState& material) {
        RenderBatch batch;
        batch.material = material;
        batch.vertices.reserve(4096);
        batch.indices.reserve(8192);
        batches_.push_back(std::move(batch));
        return &batches_.back();
    }

    void uploadBatch(RenderBatch& batch) {
        // In real impl: glBufferData for VBO/IBO
        batch.uploaded = true;
        LOGD_BR("Uploaded batch: %zu vertices, %zu indices",
                batch.vertices.size(), batch.indices.size());
    }

    void bindMaterialState(const BatchMaterialState& material) {
        // In real impl:
        // glUseProgram(material.shaderProgramId)
        // glBindTexture(GL_TEXTURE_2D, material.textureId)
        // Set blend mode, depth write, cull mode
        (void)material;
    }
};

} // namespace engine
