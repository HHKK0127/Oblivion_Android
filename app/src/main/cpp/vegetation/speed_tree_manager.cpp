// ============================================================================
// SpeedTreeManager - Billboard-based vegetation rendering system
// Phase 51: SpeedTree alternative using billboard + wind shader approach
// ============================================================================

#include "speed_tree_manager.h"
#include "tree_billboard_shader.h"
#include "tree_wind_field.h"
#include "../engine/renderer.h"
#include "../engine/shader.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <android/log.h>

#undef LOG_TAG
#define LOG_TAG "SpeedTreeManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace vegetation {

// ============================================================================
// Singleton
// ============================================================================

SpeedTreeManager& SpeedTreeManager::instance() {
    static SpeedTreeManager inst;
    return inst;
}

SpeedTreeManager::SpeedTreeManager() = default;

SpeedTreeManager::~SpeedTreeManager() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool SpeedTreeManager::initialize(Renderer* renderer) {
    if (initialized_) {
        LOGW("SpeedTreeManager already initialized");
        return true;
    }

    renderer_ = renderer;
    if (!renderer_) {
        LOGE("SpeedTreeManager: null renderer");
        return false;
    }

    // Initialize shaders
    if (!initShaders()) {
        LOGE("SpeedTreeManager: shader initialization failed");
        return false;
    }

    // Generate LOD meshes
    generateBillboardMesh();
    generateNearMesh();
    generateMidMesh();
    generateFarMesh();

    initialized_ = true;
    LOGI("SpeedTreeManager initialized successfully");
    return true;
}

void SpeedTreeManager::shutdown() {
    if (!initialized_) return;

    // Delete GL resources
    if (billboardVao_) { glDeleteVertexArrays(1, &billboardVao_); billboardVao_ = 0; }
    if (billboardVbo_) { glDeleteBuffers(1, &billboardVbo_); billboardVbo_ = 0; }
    if (nearVao_) { glDeleteVertexArrays(1, &nearVao_); nearVao_ = 0; }
    if (nearVbo_) { glDeleteBuffers(1, &nearVbo_); nearVbo_ = 0; }
    if (nearEbo_) { glDeleteBuffers(1, &nearEbo_); nearEbo_ = 0; }
    if (midVao_) { glDeleteVertexArrays(1, &midVao_); midVao_ = 0; }
    if (midVbo_) { glDeleteBuffers(1, &midVbo_); midVbo_ = 0; }
    if (midEbo_) { glDeleteBuffers(1, &midEbo_); midEbo_ = 0; }
    if (farVao_) { glDeleteVertexArrays(1, &farVao_); farVao_ = 0; }
    if (farVbo_) { glDeleteBuffers(1, &farVbo_); farVbo_ = 0; }
    if (farEbo_) { glDeleteBuffers(1, &farEbo_); farEbo_ = 0; }

    // Clean up instance buffers
    for (auto& pair : instanceBuffers_) {
        InstanceBuffer& buf = pair.second;
        if (buf.vao) { glDeleteVertexArrays(1, &buf.vao); }
        if (buf.instanceVbo) { glDeleteBuffers(1, &buf.instanceVbo); }
        if (buf.meshVbo) { glDeleteBuffers(1, &buf.meshVbo); }
        if (buf.ebo) { glDeleteBuffers(1, &buf.ebo); }
    }
    instanceBuffers_.clear();

    billboardShader_.reset();
    meshShader_.reset();

    treeTypes_.clear();
    instances_.clear();
    forestRegions_.clear();

    initialized_ = false;
    LOGI("SpeedTreeManager shut down");
}

// ============================================================================
// Shader initialization
// ============================================================================

bool SpeedTreeManager::initShaders() {
    // Billboard shader
    billboardShader_ = std::make_unique<ShaderProgram>();
    if (!billboardShader_->compile(shaders::BILLBOARD_VERTEX,
                                    shaders::BILLBOARD_FRAGMENT)) {
        LOGE("Failed to compile billboard shader");
        billboardShader_.reset();
        return false;
    }

    // Mesh shader (for NEAR/MID/FAR LOD)
    meshShader_ = std::make_unique<ShaderProgram>();
    if (!meshShader_->compile(shaders::MESH_VERTEX,
                               shaders::MESH_FRAGMENT)) {
        LOGE("Failed to compile mesh shader");
        meshShader_.reset();
        return false;
    }

    LOGI("Vegetation shaders compiled successfully");
    return true;
}

// ============================================================================
// Tree type registration
// ============================================================================

bool SpeedTreeManager::registerTreeType(uint32_t typeId, const TreeType& type) {
    if (treeTypes_.find(typeId) != treeTypes_.end()) {
        LOGW("Tree type %u already registered, overwriting", typeId);
    }

    TreeType newType = type;
    newType.typeId = typeId;
    treeTypes_[typeId] = newType;

    LOGI("Registered tree type %u: height range [%.1f, %.1f]",
         typeId, type.minHeight, type.maxHeight);
    return true;
}

const TreeType* SpeedTreeManager::getTreeType(uint32_t typeId) const {
    auto it = treeTypes_.find(typeId);
    if (it != treeTypes_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Instance management
// ============================================================================

uint32_t SpeedTreeManager::spawnTree(uint32_t typeId,
                                      const glm::vec3& pos,
                                      float rotation,
                                      float scale) {
    if (treeTypes_.find(typeId) == treeTypes_.end()) {
        LOGW("Cannot spawn tree: type %u not registered", typeId);
        return 0;
    }

    TreeInstance instance;
    instance.instanceId = nextInstanceId_++;
    instance.typeId = typeId;
    instance.position = pos;
    instance.rotation = rotation;
    instance.scale = scale;
    instance.currentLod = TreeLodLevel::BILLBOARD;
    instance.lodBlend = 0.0f;
    instance.visible = true;

    instances_.push_back(instance);

    // Mark instance buffer as dirty for this type
    auto bufIt = instanceBuffers_.find(typeId);
    if (bufIt != instanceBuffers_.end()) {
        bufIt->second.dirty = true;
    }

    return instance.instanceId;
}

bool SpeedTreeManager::removeTree(uint32_t instanceId) {
    auto it = std::find_if(instances_.begin(), instances_.end(),
        [instanceId](const TreeInstance& inst) {
            return inst.instanceId == instanceId;
        });

    if (it == instances_.end()) {
        LOGW("Cannot remove tree: instance %u not found", instanceId);
        return false;
    }

    uint32_t typeId = it->typeId;
    instances_.erase(it);

    // Mark instance buffer as dirty
    auto bufIt = instanceBuffers_.find(typeId);
    if (bufIt != instanceBuffers_.end()) {
        bufIt->second.dirty = true;
    }

    // Remove from forest regions
    for (auto& region : forestRegions_) {
        auto& ids = region.second;
        ids.erase(std::remove(ids.begin(), ids.end(), instanceId), ids.end());
    }

    return true;
}

TreeInstance* SpeedTreeManager::getTreeInstance(uint32_t instanceId) {
    for (auto& inst : instances_) {
        if (inst.instanceId == instanceId) {
            return &inst;
        }
    }
    return nullptr;
}

// ============================================================================
// Bulk spawning
// ============================================================================

size_t SpeedTreeManager::spawnForest(const std::string& worldspaceId,
                                      const std::vector<TreeInstance>& trees) {
    size_t spawned = 0;
    std::vector<uint32_t> regionIds;

    for (const auto& tree : trees) {
        if (treeTypes_.find(tree.typeId) == treeTypes_.end()) {
            LOGW("Skipping tree: type %u not registered", tree.typeId);
            continue;
        }

        TreeInstance instance = tree;
        instance.instanceId = nextInstanceId_++;
        instance.currentLod = TreeLodLevel::BILLBOARD;
        instance.lodBlend = 0.0f;
        instance.visible = true;

        instances_.push_back(instance);
        regionIds.push_back(instance.instanceId);
        spawned++;
    }

    forestRegions_[worldspaceId] = regionIds;

    // Mark all affected type buffers as dirty
    for (const auto& tree : trees) {
        auto bufIt = instanceBuffers_.find(tree.typeId);
        if (bufIt != instanceBuffers_.end()) {
            bufIt->second.dirty = true;
        }
    }

    LOGI("Spawned %lu trees for region '%s'", (unsigned long)spawned,
         worldspaceId.c_str());
    return spawned;
}

void SpeedTreeManager::unloadForest(const std::string& worldspaceId) {
    auto it = forestRegions_.find(worldspaceId);
    if (it == forestRegions_.end()) {
        LOGW("Forest region '%s' not found", worldspaceId.c_str());
        return;
    }

    const auto& ids = it->second;
    size_t removed = 0;

    for (uint32_t id : ids) {
        auto instIt = std::find_if(instances_.begin(), instances_.end(),
            [id](const TreeInstance& inst) {
                return inst.instanceId == id;
            });
        if (instIt != instances_.end()) {
            instances_.erase(instIt);
            removed++;
        }
    }

    forestRegions_.erase(it);
    LOGI("Unloaded %lu trees from region '%s'", (unsigned long)removed,
         worldspaceId.c_str());
}

// ============================================================================
// LOD selection
// ============================================================================

TreeLodLevel SpeedTreeManager::selectLod(const glm::vec3& treePos,
                                          const glm::vec3& cameraPos) const {
    float dist = glm::length(cameraPos - treePos);

    if (dist < lodNearDist_) {
        return TreeLodLevel::NEAR;
    } else if (dist < lodMidDist_) {
        return TreeLodLevel::MID;
    } else if (dist < lodFarDist_) {
        return TreeLodLevel::FAR;
    } else {
        return TreeLodLevel::BILLBOARD;
    }
}

void SpeedTreeManager::setLodDistances(float nearDist, float midDist, float farDist) {
    lodNearDist_ = nearDist;
    lodMidDist_ = midDist;
    lodFarDist_ = farDist;
}

// ============================================================================
// Update (wind + LOD)
// ============================================================================

void SpeedTreeManager::update(float deltaTime, const glm::vec3& windDir) {
    if (!initialized_) return;

    // Update wind time
    windTime_ += deltaTime;

    // Update wind direction
    windParams_.direction = glm::normalize(windDir);
}

// ============================================================================
// Frustum culling
// ============================================================================

bool SpeedTreeManager::isInFrustum(const glm::vec3& pos, float radius,
                                    const glm::mat4& viewProj) const {
    // Extract frustum planes from view-projection matrix
    // Left plane
    glm::vec4 leftPlane = glm::vec4(
        viewProj[0][3] + viewProj[0][0],
        viewProj[1][3] + viewProj[1][0],
        viewProj[2][3] + viewProj[2][0],
        viewProj[3][3] + viewProj[3][0]
    );
    float leftLen = glm::length(glm::vec3(leftPlane));
    if (leftLen > 0.001f) {
        leftPlane /= leftLen;
        float d = leftPlane.x * pos.x + leftPlane.y * pos.y +
                  leftPlane.z * pos.z + leftPlane.w;
        if (d < -radius) return false;
    }

    // Right plane
    glm::vec4 rightPlane = glm::vec4(
        viewProj[0][3] - viewProj[0][0],
        viewProj[1][3] - viewProj[1][0],
        viewProj[2][3] - viewProj[2][0],
        viewProj[3][3] - viewProj[3][0]
    );
    float rightLen = glm::length(glm::vec3(rightPlane));
    if (rightLen > 0.001f) {
        rightPlane /= rightLen;
        float d = rightPlane.x * pos.x + rightPlane.y * pos.y +
                  rightPlane.z * pos.z + rightPlane.w;
        if (d < -radius) return false;
    }

    // Bottom plane
    glm::vec4 bottomPlane = glm::vec4(
        viewProj[0][3] + viewProj[0][1],
        viewProj[1][3] + viewProj[1][1],
        viewProj[2][3] + viewProj[2][1],
        viewProj[3][3] + viewProj[3][1]
    );
    float bottomLen = glm::length(glm::vec3(bottomPlane));
    if (bottomLen > 0.001f) {
        bottomPlane /= bottomLen;
        float d = bottomPlane.x * pos.x + bottomPlane.y * pos.y +
                  bottomPlane.z * pos.z + bottomPlane.w;
        if (d < -radius) return false;
    }

    // Top plane
    glm::vec4 topPlane = glm::vec4(
        viewProj[0][3] - viewProj[0][1],
        viewProj[1][3] - viewProj[1][1],
        viewProj[2][3] - viewProj[2][1],
        viewProj[3][3] - viewProj[3][1]
    );
    float topLen = glm::length(glm::vec3(topPlane));
    if (topLen > 0.001f) {
        topPlane /= topLen;
        float d = topPlane.x * pos.x + topPlane.y * pos.y +
                  topPlane.z * pos.z + topPlane.w;
        if (d < -radius) return false;
    }

    // Near plane
    glm::vec4 nearPlane = glm::vec4(
        viewProj[0][3] + viewProj[0][2],
        viewProj[1][3] + viewProj[1][2],
        viewProj[2][3] + viewProj[2][2],
        viewProj[3][3] + viewProj[3][2]
    );
    float nearLen = glm::length(glm::vec3(nearPlane));
    if (nearLen > 0.001f) {
        nearPlane /= nearLen;
        float d = nearPlane.x * pos.x + nearPlane.y * pos.y +
                  nearPlane.z * pos.z + nearPlane.w;
        if (d < -radius) return false;
    }

    // Far plane
    glm::vec4 farPlane = glm::vec4(
        viewProj[0][3] - viewProj[0][2],
        viewProj[1][3] - viewProj[1][2],
        viewProj[2][3] - viewProj[2][2],
        viewProj[3][3] - viewProj[3][2]
    );
    float farLen = glm::length(glm::vec3(farPlane));
    if (farLen > 0.001f) {
        farPlane /= farLen;
        float d = farPlane.x * pos.x + farPlane.y * pos.y +
                  farPlane.z * pos.z + farPlane.w;
        if (d < -radius) return false;
    }

    return true;
}

// ============================================================================
// Render
// ============================================================================

void SpeedTreeManager::render(Renderer* renderer,
                               const glm::mat4& viewProj,
                               const glm::vec3& cameraPos) {
    if (!initialized_ || instances_.empty()) return;

    (void)renderer;
    visibleCount_ = 0;
    drawCalls_ = 0;

    // Classify trees by LOD and visibility
    std::vector<TreeInstance*> nearTrees;
    std::vector<TreeInstance*> midTrees;
    std::vector<TreeInstance*> farTrees;
    std::vector<TreeInstance*> billboardTrees;

    for (auto& inst : instances_) {
        // Distance check
        float dist = glm::length(cameraPos - inst.position);
        if (dist > maxDrawDistance_) {
            inst.visible = false;
            continue;
        }

        // Frustum culling (use bounding sphere radius = scale * 10)
        float boundingRadius = inst.scale * 10.0f;
        if (!isInFrustum(inst.position, boundingRadius, viewProj)) {
            inst.visible = false;
            continue;
        }

        inst.visible = true;
        inst.currentLod = selectLod(inst.position, cameraPos);
        visibleCount_++;

        switch (inst.currentLod) {
            case TreeLodLevel::NEAR:
                nearTrees.push_back(&inst);
                break;
            case TreeLodLevel::MID:
                midTrees.push_back(&inst);
                break;
            case TreeLodLevel::FAR:
                farTrees.push_back(&inst);
                break;
            case TreeLodLevel::BILLBOARD:
                billboardTrees.push_back(&inst);
                break;
        }
    }

    // Render each LOD group
    if (!nearTrees.empty()) renderNearLod(nearTrees, viewProj, cameraPos);
    if (!midTrees.empty()) renderMidLod(midTrees, viewProj, cameraPos);
    if (!farTrees.empty()) renderFarLod(farTrees, viewProj, cameraPos);
    if (!billboardTrees.empty()) renderBillboards(billboardTrees, viewProj, cameraPos);
}

// ============================================================================
// Billboard rendering
// ============================================================================

void SpeedTreeManager::renderBillboards(const std::vector<TreeInstance*>& trees,
                                         const glm::mat4& viewProj,
                                         const glm::vec3& cameraPos) {
    if (!billboardShader_ || !billboardShader_->isValid()) return;
    if (billboardVao_ == 0) return;

    billboardShader_->use();
    billboardShader_->setUniform("uViewProj", viewProj);
    billboardShader_->setUniform("uCameraPos", cameraPos);
    billboardShader_->setUniform("uWindDirection", windParams_.direction);
    billboardShader_->setUniform("uWindStrength", windParams_.strength);
    billboardShader_->setUniform("uWindTime", windTime_);
    billboardShader_->setUniform("uSwayPeriod", windParams_.swayPeriod);
    billboardShader_->setUniform("uVertexAmplitude", windParams_.vertexAmplitude);
    billboardShader_->setUniform("uMaxDrawDistance", maxDrawDistance_);
    billboardShader_->setUniform("uFadeStartDist", maxDrawDistance_ * 0.8f);
    billboardShader_->setUniform("uFogColor", glm::vec4(0.5f, 0.6f, 0.7f, 1.0f));
    billboardShader_->setUniform("uAlphaThreshold", 0.3f);

    // Enable blending for alpha
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Group by type for texture binding
    std::unordered_map<uint32_t, std::vector<const TreeInstance*>> byType;
    for (const auto* tree : trees) {
        byType[tree->typeId].push_back(tree);
    }

    for (const auto& group : byType) {
        const TreeType* type = getTreeType(group.first);
        if (!type) continue;

        // Bind texture
        GLuint texId = type->billboardTextureId ? type->billboardTextureId : type->textureId;
        if (texId) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texId);
            billboardShader_->setUniform("uTexture", 0);
        }

        // Render each billboard as individual draw calls
        // (For production, these would be instanced)
        glBindVertexArray(billboardVao_);

        for (const auto* tree : group.second) {
            // Set per-tree uniforms (simplified - in production use instancing)
            float windOffset = static_cast<float>(tree->instanceId % 100) * 0.1f;

            // Compute billboard vertices manually for this tree
            glm::vec3 toCamera = glm::normalize(cameraPos - tree->position);
            glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), toCamera));
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            float halfW = type->billboardWidth * 0.5f * tree->scale;
            float h = type->billboardHeight * tree->scale;

            // Wind sway
            float heightFactor = 1.0f;
            float windPhase = windTime_ * windParams_.swayPeriod + windOffset;
            float sway = std::sin(windPhase) * windParams_.vertexAmplitude * windParams_.strength;
            glm::vec3 windOff = windParams_.direction * sway;

            // Billboard quad vertices (two triangles)
            float bbVertices[] = {
                // pos (3) + texcoord (2)
                tree->position.x + right.x * (-halfW) + up.x * 0.0f + windOff.x * 0.0f,
                tree->position.y + right.y * (-halfW) + up.y * 0.0f + windOff.y * 0.0f,
                tree->position.z + right.z * (-halfW) + up.z * 0.0f + windOff.z * 0.0f,
                0.0f, 0.0f,

                tree->position.x + right.x * halfW + up.x * 0.0f + windOff.x * 0.0f,
                tree->position.y + right.y * halfW + up.y * 0.0f + windOff.y * 0.0f,
                tree->position.z + right.z * halfW + up.z * 0.0f + windOff.z * 0.0f,
                1.0f, 0.0f,

                tree->position.x + right.x * halfW + up.x * h + windOff.x * heightFactor,
                tree->position.y + right.y * halfW + up.y * h + windOff.y * heightFactor,
                tree->position.z + right.z * halfW + up.z * h + windOff.z * heightFactor,
                1.0f, 1.0f,

                tree->position.x + right.x * (-halfW) + up.x * 0.0f + windOff.x * 0.0f,
                tree->position.y + right.y * (-halfW) + up.y * 0.0f + windOff.y * 0.0f,
                tree->position.z + right.z * (-halfW) + up.z * 0.0f + windOff.z * 0.0f,
                0.0f, 0.0f,

                tree->position.x + right.x * halfW + up.x * h + windOff.x * heightFactor,
                tree->position.y + right.y * halfW + up.y * h + windOff.y * heightFactor,
                tree->position.z + right.z * halfW + up.z * h + windOff.z * heightFactor,
                1.0f, 1.0f,

                tree->position.x + right.x * (-halfW) + up.x * h + windOff.x * heightFactor,
                tree->position.y + right.y * (-halfW) + up.y * h + windOff.y * heightFactor,
                tree->position.z + right.z * (-halfW) + up.z * h + windOff.z * heightFactor,
                0.0f, 1.0f,
            };

            glBindBuffer(GL_ARRAY_BUFFER, billboardVbo_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(bbVertices), bbVertices, GL_DYNAMIC_DRAW);

            // Position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // TexCoord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                  (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            drawCalls_++;
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

// ============================================================================
// Mesh LOD rendering (NEAR/MID/FAR)
// ============================================================================

void SpeedTreeManager::renderNearLod(const std::vector<TreeInstance*>& trees,
                                      const glm::mat4& viewProj,
                                      const glm::vec3& cameraPos) {
    if (!meshShader_ || !meshShader_->isValid()) return;
    if (nearVao_ == 0) return;

    meshShader_->use();
    meshShader_->setUniform("uViewProj", viewProj);
    meshShader_->setUniform("uCameraPos", cameraPos);
    meshShader_->setUniform("uWindDirection", windParams_.direction);
    meshShader_->setUniform("uWindStrength", windParams_.strength);
    meshShader_->setUniform("uWindTime", windTime_);
    meshShader_->setUniform("uSwayPeriod", windParams_.swayPeriod);
    meshShader_->setUniform("uVertexAmplitude", windParams_.vertexAmplitude);
    meshShader_->setUniform("uLightDir", glm::vec3(0.5f, 0.8f, 0.3f));
    meshShader_->setUniform("uFogColor", glm::vec4(0.5f, 0.6f, 0.7f, 1.0f));
    meshShader_->setUniform("uAlphaThreshold", 0.3f);

    glBindVertexArray(nearVao_);

    for (const auto* tree : trees) {
        const TreeType* type = getTreeType(tree->typeId);
        if (!type) continue;

        // Bind texture
        if (type->textureId) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, type->textureId);
            meshShader_->setUniform("uTexture", 0);
        }

        // Per-tree uniforms (simplified - production would use instancing)
        float windOffset = static_cast<float>(tree->instanceId % 100) * 0.1f;
        (void)windOffset;

        // Draw near mesh (simplified cylinder + cone for now)
        glDrawElements(GL_TRIANGLES, 48, GL_UNSIGNED_INT, 0);
        drawCalls_++;
    }

    glBindVertexArray(0);
}

void SpeedTreeManager::renderMidLod(const std::vector<TreeInstance*>& trees,
                                     const glm::mat4& viewProj,
                                     const glm::vec3& cameraPos) {
    if (!meshShader_ || !meshShader_->isValid()) return;
    if (midVao_ == 0) return;

    meshShader_->use();
    meshShader_->setUniform("uViewProj", viewProj);
    meshShader_->setUniform("uCameraPos", cameraPos);
    meshShader_->setUniform("uWindDirection", windParams_.direction);
    meshShader_->setUniform("uWindStrength", windParams_.strength);
    meshShader_->setUniform("uWindTime", windTime_);
    meshShader_->setUniform("uSwayPeriod", windParams_.swayPeriod);
    meshShader_->setUniform("uVertexAmplitude", windParams_.vertexAmplitude * 0.7f);
    meshShader_->setUniform("uLightDir", glm::vec3(0.5f, 0.8f, 0.3f));
    meshShader_->setUniform("uFogColor", glm::vec4(0.5f, 0.6f, 0.7f, 1.0f));
    meshShader_->setUniform("uAlphaThreshold", 0.3f);

    glBindVertexArray(midVao_);

    for (const auto* tree : trees) {
        const TreeType* type = getTreeType(tree->typeId);
        if (!type) continue;

        if (type->textureId) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, type->textureId);
            meshShader_->setUniform("uTexture", 0);
        }

        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
        drawCalls_++;
    }

    glBindVertexArray(0);
}

void SpeedTreeManager::renderFarLod(const std::vector<TreeInstance*>& trees,
                                     const glm::mat4& viewProj,
                                     const glm::vec3& cameraPos) {
    if (!meshShader_ || !meshShader_->isValid()) return;
    if (farVao_ == 0) return;

    meshShader_->use();
    meshShader_->setUniform("uViewProj", viewProj);
    meshShader_->setUniform("uCameraPos", cameraPos);
    meshShader_->setUniform("uWindDirection", windParams_.direction);
    meshShader_->setUniform("uWindStrength", windParams_.strength * 0.5f);
    meshShader_->setUniform("uWindTime", windTime_);
    meshShader_->setUniform("uSwayPeriod", windParams_.swayPeriod);
    meshShader_->setUniform("uVertexAmplitude", windParams_.vertexAmplitude * 0.3f);
    meshShader_->setUniform("uLightDir", glm::vec3(0.5f, 0.8f, 0.3f));
    meshShader_->setUniform("uFogColor", glm::vec4(0.5f, 0.6f, 0.7f, 1.0f));
    meshShader_->setUniform("uAlphaThreshold", 0.3f);

    glBindVertexArray(farVao_);

    for (const auto* tree : trees) {
        const TreeType* type = getTreeType(tree->typeId);
        if (!type) continue;

        if (type->textureId) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, type->textureId);
            meshShader_->setUniform("uTexture", 0);
        }

        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
        drawCalls_++;
    }

    glBindVertexArray(0);
}

// ============================================================================
// Mesh generation
// ============================================================================

void SpeedTreeManager::generateBillboardMesh() {
    // Billboard is rendered dynamically per-tree in renderBillboards()
    // Just create a placeholder VAO/VBO
    glGenVertexArrays(1, &billboardVao_);
    glGenBuffers(1, &billboardVbo_);

    glBindVertexArray(billboardVao_);
    glBindBuffer(GL_ARRAY_BUFFER, billboardVbo_);

    // Placeholder quad (will be overwritten per draw call)
    float placeholder[30] = {};
    glBufferData(GL_ARRAY_BUFFER, sizeof(placeholder), placeholder, GL_DYNAMIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void SpeedTreeManager::generateNearMesh() {
    // Near LOD: cylinder trunk + cone crown (~500 polys simulated with lower count)
    // Simplified: 8-sided cylinder (trunk) + 8-sided cone (crown)
    const int sides = 8;
    const float trunkRadius = 0.3f;
    const float trunkHeight = 2.0f;
    const float crownRadius = 1.5f;
    const float crownHeight = 4.0f;
    const float totalHeight = trunkHeight + crownHeight;

    // Vertices: pos(3) + texcoord(2) + normal(3) = 8 floats per vertex
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Trunk cylinder
    for (int i = 0; i <= sides; i++) {
        float angle = static_cast<float>(i) / sides * 2.0f * 3.14159265f;
        float x = std::cos(angle) * trunkRadius;
        float z = std::sin(angle) * trunkRadius;
        float u = static_cast<float>(i) / sides;

        // Bottom vertex
        vertices.insert(vertices.end(), {x, 0.0f, z, u, 0.0f, x, 0.0f, z});
        // Top vertex
        vertices.insert(vertices.end(), {x, trunkHeight, z, u, 0.3f, x, 0.0f, z});
    }

    // Trunk indices
    for (int i = 0; i < sides; i++) {
        unsigned int base = static_cast<unsigned int>(i * 2);
        indices.insert(indices.end(), {base, base + 1, base + 2});
        indices.insert(indices.end(), {base + 1, base + 3, base + 2});
    }

    // Crown cone
    unsigned int crownBase = static_cast<unsigned int>(vertices.size() / 8);
    // Apex
    vertices.insert(vertices.end(), {0.0f, totalHeight, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f});
    for (int i = 0; i <= sides; i++) {
        float angle = static_cast<float>(i) / sides * 2.0f * 3.14159265f;
        float x = std::cos(angle) * crownRadius;
        float z = std::sin(angle) * crownRadius;
        float u = static_cast<float>(i) / sides;
        float nx = x / crownRadius;
        float nz = z / crownRadius;
        vertices.insert(vertices.end(),
            {x, trunkHeight, z, u, 0.3f, nx, 0.5f, nz});
    }

    // Crown indices
    for (int i = 0; i < sides; i++) {
        indices.insert(indices.end(), {
            crownBase,
            crownBase + 1 + static_cast<unsigned int>(i),
            crownBase + 2 + static_cast<unsigned int>(i)
        });
    }

    glGenVertexArrays(1, &nearVao_);
    glGenBuffers(1, &nearVbo_);
    glGenBuffers(1, &nearEbo_);

    glBindVertexArray(nearVao_);

    glBindBuffer(GL_ARRAY_BUFFER, nearVbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nearEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void SpeedTreeManager::generateMidMesh() {
    // Mid LOD: simplified 6-sided cylinder + pyramid
    const int sides = 6;
    const float trunkRadius = 0.3f;
    const float trunkHeight = 2.0f;
    const float crownRadius = 1.2f;
    const float crownHeight = 3.0f;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Simplified trunk
    for (int i = 0; i <= sides; i++) {
        float angle = static_cast<float>(i) / sides * 2.0f * 3.14159265f;
        float x = std::cos(angle) * trunkRadius;
        float z = std::sin(angle) * trunkRadius;
        float u = static_cast<float>(i) / sides;
        vertices.insert(vertices.end(), {x, 0.0f, z, u, 0.0f, x, 0.0f, z});
        vertices.insert(vertices.end(), {x, trunkHeight, z, u, 0.3f, x, 0.0f, z});
    }

    for (int i = 0; i < sides; i++) {
        unsigned int base = static_cast<unsigned int>(i * 2);
        indices.insert(indices.end(), {base, base + 1, base + 2});
        indices.insert(indices.end(), {base + 1, base + 3, base + 2});
    }

    // Simplified crown (pyramid)
    float totalH = trunkHeight + crownHeight;
    unsigned int crownBase = static_cast<unsigned int>(vertices.size() / 8);
    vertices.insert(vertices.end(), {0.0f, totalH, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f});
    for (int i = 0; i <= sides; i++) {
        float angle = static_cast<float>(i) / sides * 2.0f * 3.14159265f;
        float x = std::cos(angle) * crownRadius;
        float z = std::sin(angle) * crownRadius;
        float u = static_cast<float>(i) / sides;
        vertices.insert(vertices.end(), {x, trunkHeight, z, u, 0.3f, x, 0.5f, z});
    }

    for (int i = 0; i < sides; i++) {
        indices.insert(indices.end(), {
            crownBase,
            crownBase + 1 + static_cast<unsigned int>(i),
            crownBase + 2 + static_cast<unsigned int>(i)
        });
    }

    glGenVertexArrays(1, &midVao_);
    glGenBuffers(1, &midVbo_);
    glGenBuffers(1, &midEbo_);

    glBindVertexArray(midVao_);

    glBindBuffer(GL_ARRAY_BUFFER, midVbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, midEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void SpeedTreeManager::generateFarMesh() {
    // Far LOD: very simple 4-sided pyramid
    const int sides = 4;
    const float trunkRadius = 0.25f;
    const float trunkHeight = 1.5f;
    const float crownRadius = 1.0f;
    const float crownHeight = 2.5f;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= sides; i++) {
        float angle = static_cast<float>(i) / sides * 2.0f * 3.14159265f;
        float x = std::cos(angle) * trunkRadius;
        float z = std::sin(angle) * trunkRadius;
        float u = static_cast<float>(i) / sides;
        vertices.insert(vertices.end(), {x, 0.0f, z, u, 0.0f, x, 0.0f, z});
        vertices.insert(vertices.end(), {x, trunkHeight, z, u, 0.4f, x, 0.0f, z});
    }

    for (int i = 0; i < sides; i++) {
        unsigned int base = static_cast<unsigned int>(i * 2);
        indices.insert(indices.end(), {base, base + 1, base + 2});
        indices.insert(indices.end(), {base + 1, base + 3, base + 2});
    }

    float totalH = trunkHeight + crownHeight;
    unsigned int crownBase = static_cast<unsigned int>(vertices.size() / 8);
    vertices.insert(vertices.end(), {0.0f, totalH, 0.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f});
    for (int i = 0; i <= sides; i++) {
        float angle = static_cast<float>(i) / sides * 2.0f * 3.14159265f;
        float x = std::cos(angle) * crownRadius;
        float z = std::sin(angle) * crownRadius;
        float u = static_cast<float>(i) / sides;
        vertices.insert(vertices.end(), {x, trunkHeight, z, u, 0.4f, x, 0.5f, z});
    }

    for (int i = 0; i < sides; i++) {
        indices.insert(indices.end(), {
            crownBase,
            crownBase + 1 + static_cast<unsigned int>(i),
            crownBase + 2 + static_cast<unsigned int>(i)
        });
    }

    glGenVertexArrays(1, &farVao_);
    glGenBuffers(1, &farVbo_);
    glGenBuffers(1, &farEbo_);

    glBindVertexArray(farVao_);

    glBindBuffer(GL_ARRAY_BUFFER, farVbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, farEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

} // namespace vegetation
