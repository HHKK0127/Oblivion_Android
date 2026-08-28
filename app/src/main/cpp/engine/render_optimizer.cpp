#include "render_optimizer.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// RenderOptimizer Implementation
// ============================================================================

RenderOptimizer::RenderOptimizer() = default;

RenderOptimizer::~RenderOptimizer() {
    cleanup();
}

bool RenderOptimizer::initialize() {
    if (initialized_) return true;

    commands_.reserve(maxCommands_);
    batches_.reserve(256);

    // Create instance VBO for GPU instancing
    glGenBuffers(1, &instanceVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);
    // Pre-allocate for 256 instances
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    initialized_ = true;
    LOGI_RO("RenderOptimizer initialized (maxCommands=%u)", maxCommands_);
    return true;
}

void RenderOptimizer::cleanup() {
    if (!initialized_) return;

    if (instanceVBO_ != 0) {
        glDeleteBuffers(1, &instanceVBO_);
        instanceVBO_ = 0;
    }

    commands_.clear();
    batches_.clear();
    initialized_ = false;
    LOGI_RO("RenderOptimizer cleaned up");
}

void RenderOptimizer::beginFrame() {
    commands_.clear();
    stats_ = {};
}

void RenderOptimizer::submitCommand(const RenderCommand& cmd) {
    if (commands_.size() >= maxCommands_) {
        LOGW_RO("Max commands reached (%u), dropping command", maxCommands_);
        return;
    }
    commands_.push_back(cmd);
}

void RenderOptimizer::processCommands(const float* viewProjMatrix, const float* cameraPos) {
    // Update frustum for culling
    if (frustumCullingEnabled_ && viewProjMatrix) {
        updateFrustum(viewProjMatrix);
    }

    stats_.totalCommands = static_cast<uint32_t>(commands_.size());

    // Frustum culling pass
    if (frustumCullingEnabled_) {
        auto it = std::remove_if(commands_.begin(), commands_.end(),
            [this](const RenderCommand& cmd) {
                if (!isSphereVisible(cmd.position, cmd.boundingRadius)) {
                    stats_.frustumCulled++;
                    return true;
                }
                return false;
            });
        commands_.erase(it, commands_.end());
    }

    // Sort for batching
    sortCommands();

    // Create batches
    createBatches();

    // Calculate stats
    uint32_t totalInstances = 0;
    for (const auto& batch : batches_) {
        totalInstances += static_cast<uint32_t>(batch.instances.size());
    }
    stats_.batchedDrawCalls = static_cast<uint32_t>(batches_.size());
    if (stats_.totalCommands > 0) {
        stats_.savedDrawCalls = totalInstances - stats_.batchedDrawCalls;
        stats_.batchingRatio = static_cast<float>(stats_.savedDrawCalls) /
                               static_cast<float>(totalInstances) * 100.0f;
    }
    stats_.instancesRendered = totalInstances;
}

void RenderOptimizer::flush() {
    for (const auto& batch : batches_) {
        executeBatch(batch);
    }
}

// ============================================================================
// Frustum Culling
// ============================================================================

void RenderOptimizer::updateFrustum(const float* viewProjMatrix) {
    // Extract frustum planes from view-projection matrix (column-major)
    // Left plane
    frustumPlanes_[0].normal[0] = viewProjMatrix[3] + viewProjMatrix[0];
    frustumPlanes_[0].normal[1] = viewProjMatrix[7] + viewProjMatrix[4];
    frustumPlanes_[0].normal[2] = viewProjMatrix[11] + viewProjMatrix[8];
    frustumPlanes_[0].distance = viewProjMatrix[15] + viewProjMatrix[12];

    // Right plane
    frustumPlanes_[1].normal[0] = viewProjMatrix[3] - viewProjMatrix[0];
    frustumPlanes_[1].normal[1] = viewProjMatrix[7] - viewProjMatrix[4];
    frustumPlanes_[1].normal[2] = viewProjMatrix[11] - viewProjMatrix[8];
    frustumPlanes_[1].distance = viewProjMatrix[15] - viewProjMatrix[12];

    // Bottom plane
    frustumPlanes_[2].normal[0] = viewProjMatrix[3] + viewProjMatrix[1];
    frustumPlanes_[2].normal[1] = viewProjMatrix[7] + viewProjMatrix[5];
    frustumPlanes_[2].normal[2] = viewProjMatrix[11] + viewProjMatrix[9];
    frustumPlanes_[2].distance = viewProjMatrix[15] + viewProjMatrix[13];

    // Top plane
    frustumPlanes_[3].normal[0] = viewProjMatrix[3] - viewProjMatrix[1];
    frustumPlanes_[3].normal[1] = viewProjMatrix[7] - viewProjMatrix[5];
    frustumPlanes_[3].normal[2] = viewProjMatrix[11] - viewProjMatrix[9];
    frustumPlanes_[3].distance = viewProjMatrix[15] - viewProjMatrix[13];

    // Near plane
    frustumPlanes_[4].normal[0] = viewProjMatrix[3] + viewProjMatrix[2];
    frustumPlanes_[4].normal[1] = viewProjMatrix[7] + viewProjMatrix[6];
    frustumPlanes_[4].normal[2] = viewProjMatrix[11] + viewProjMatrix[10];
    frustumPlanes_[4].distance = viewProjMatrix[15] + viewProjMatrix[14];

    // Far plane
    frustumPlanes_[5].normal[0] = viewProjMatrix[3] - viewProjMatrix[2];
    frustumPlanes_[5].normal[1] = viewProjMatrix[7] - viewProjMatrix[6];
    frustumPlanes_[5].normal[2] = viewProjMatrix[11] - viewProjMatrix[10];
    frustumPlanes_[5].distance = viewProjMatrix[15] - viewProjMatrix[14];

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        normalizePlane(frustumPlanes_[i]);
    }
}

bool RenderOptimizer::isSphereVisible(const float* center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        float dist = frustumPlanes_[i].normal[0] * center[0] +
                     frustumPlanes_[i].normal[1] * center[1] +
                     frustumPlanes_[i].normal[2] * center[2] +
                     frustumPlanes_[i].distance;
        if (dist < -radius) {
            return false;
        }
    }
    return true;
}

void RenderOptimizer::normalizePlane(FrustumPlane& plane) {
    float length = std::sqrt(plane.normal[0] * plane.normal[0] +
                             plane.normal[1] * plane.normal[1] +
                             plane.normal[2] * plane.normal[2]);
    if (length > 0.0001f) {
        float invLen = 1.0f / length;
        plane.normal[0] *= invLen;
        plane.normal[1] *= invLen;
        plane.normal[2] *= invLen;
        plane.distance *= invLen;
    }
}

// ============================================================================
// Sorting and Batching
// ============================================================================

int32_t RenderOptimizer::generateSortKey(const RenderCommand& cmd) const {
    // Sort by: transparency (opaque first), then shader, then material, then texture
    int32_t key = 0;
    key |= (cmd.transparent ? 1 : 0) << 31;
    key |= (cmd.shaderProgram & 0xFF) << 23;
    key |= (cmd.materialId & 0xFF) << 15;
    key |= (cmd.textureId & 0x7FFF);
    return key;
}

void RenderOptimizer::sortCommands() {
    // Assign sort keys
    for (auto& cmd : commands_) {
        cmd.sortKey = generateSortKey(cmd);
    }

    // Sort by sort key for batching
    std::sort(commands_.begin(), commands_.end(),
        [](const RenderCommand& a, const RenderCommand& b) {
            return a.sortKey < b.sortKey;
        });
}

void RenderOptimizer::createBatches() {
    batches_.clear();

    if (commands_.empty()) return;

    BatchGroup currentBatch;
    currentBatch.materialId = commands_[0].materialId;
    currentBatch.shaderProgram = commands_[0].shaderProgram;
    currentBatch.textureId = commands_[0].textureId;
    currentBatch.vao = commands_[0].vao;
    currentBatch.vbo = commands_[0].vbo;
    currentBatch.ebo = commands_[0].ebo;
    currentBatch.indexCount = commands_[0].indexCount;

    for (const auto& cmd : commands_) {
        // Check if this command can be batched with current group
        if (cmd.materialId != currentBatch.materialId ||
            cmd.shaderProgram != currentBatch.shaderProgram ||
            cmd.textureId != currentBatch.textureId ||
            cmd.vao != currentBatch.vao) {
            // Flush current batch
            if (!currentBatch.instances.empty()) {
                batches_.push_back(std::move(currentBatch));
            }
            // Start new batch
            currentBatch = BatchGroup();
            currentBatch.materialId = cmd.materialId;
            currentBatch.shaderProgram = cmd.shaderProgram;
            currentBatch.textureId = cmd.textureId;
            currentBatch.vao = cmd.vao;
            currentBatch.vbo = cmd.vbo;
            currentBatch.ebo = cmd.ebo;
            currentBatch.indexCount = cmd.indexCount;
        }

        // Add instance data
        InstanceData instance;
        std::memcpy(instance.modelMatrix, cmd.modelMatrix, sizeof(float) * 16);
        currentBatch.instances.push_back(instance);
    }

    // Push last batch
    if (!currentBatch.instances.empty()) {
        batches_.push_back(std::move(currentBatch));
    }
}

void RenderOptimizer::executeBatch(const BatchGroup& batch) {
    if (batch.instances.empty()) return;

    // Bind shared state once per batch
    glUseProgram(batch.shaderProgram);
    glBindVertexArray(batch.vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, batch.textureId);

    if (instancingEnabled_ && batch.instances.size() > 1) {
        // GPU instancing path
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);

        // Upload instance data
        size_t dataSize = batch.instances.size() * sizeof(InstanceData);
        glBufferData(GL_ARRAY_BUFFER, dataSize, batch.instances.data(), GL_DYNAMIC_DRAW);

        // Set up instance matrix attribute (locations 3-6 for mat4)
        for (int i = 0; i < 4; ++i) {
            GLuint loc = 3 + i;
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(InstanceData),
                                  reinterpret_cast<void*>(i * 4 * sizeof(float)));
            glVertexAttribDivisor(loc, 1);
        }

        // Draw instanced
        if (batch.ebo != 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ebo);
            glDrawElementsInstanced(GL_TRIANGLES, batch.indexCount,
                                    GL_UNSIGNED_SHORT, nullptr,
                                    static_cast<GLsizei>(batch.instances.size()));
        } else {
            glDrawArraysInstanced(GL_TRIANGLES, 0, batch.indexCount,
                                  static_cast<GLsizei>(batch.instances.size()));
        }

        // Reset divisors
        for (int i = 0; i < 4; ++i) {
            glVertexAttribDivisor(3 + i, 0);
            glDisableVertexAttribArray(3 + i);
        }

        stats_.instancesRendered += static_cast<uint32_t>(batch.instances.size());
    } else {
        // Non-instanced fallback: draw each instance individually
        for (const auto& instance : batch.instances) {
            // Set model matrix uniform (location 0 assumed)
            glUniformMatrix4fv(0, 1, GL_FALSE, instance.modelMatrix);

            if (batch.ebo != 0) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ebo);
                glDrawElements(GL_TRIANGLES, batch.indexCount,
                               GL_UNSIGNED_SHORT, nullptr);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, batch.indexCount);
            }
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void RenderOptimizer::resetStats() {
    stats_ = {};
}
