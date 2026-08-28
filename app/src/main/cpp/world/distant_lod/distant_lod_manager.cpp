#include "distant_lod_manager.h"
#include "distant_lod_shader.h"
#include "distant_lod_horizon.h"
#include "../world_manager.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <cstring>
#include <algorithm>

// ============================================================================
// DistantLodManager Implementation - Phase 50
// ============================================================================

DistantLodManager& DistantLodManager::instance() {
    static DistantLodManager inst;
    return inst;
}

// ============================================================================
// Lifecycle
// ============================================================================

bool DistantLodManager::initialize(WorldManager* worldManager, void* landLoader) {
    if (initialized_) {
        LOGW_DLOD("Already initialized, skipping");
        return true;
    }

    worldManager_ = worldManager;
    landLoader_ = landLoader;

    // Compile shaders
    if (!compileShader()) {
        LOGE_DLOD("Failed to compile distant LOD shader");
        return false;
    }

    // Generate horizon ring
    if (!generateHorizonRing()) {
        LOGW_DLOD("Failed to generate horizon ring (non-fatal)");
    }

    initialized_ = true;
    LOGI_DLOD("DistantLodManager initialized successfully");
    return true;
}

void DistantLodManager::cleanup() {
    if (!initialized_) return;

    // Release all GPU resources
    for (auto& pair : lodMeshes_) {
        releaseMeshGpu(pair.second);
    }
    lodMeshes_.clear();

    releaseHorizonGpu();
    deleteShader();

    worldManager_ = nullptr;
    landLoader_ = nullptr;
    initialized_ = false;

    LOGI_DLOD("DistantLodManager cleaned up");
}

// ============================================================================
// LOD Mesh Registration
// ============================================================================

void DistantLodManager::registerLodMesh(const std::string& worldspaceId,
                                          const LodMeshData& data) {
    if (worldspaceId.empty()) {
        LOGW_DLOD("Cannot register LOD mesh with empty ID");
        return;
    }

    // Release existing GPU resources if overwriting
    auto it = lodMeshes_.find(worldspaceId);
    if (it != lodMeshes_.end()) {
        releaseMeshGpu(it->second);
    }

    LodMeshData meshCopy = data;
    calculateBoundingSphere(meshCopy);

    // Upload to GPU
    uploadMeshToGpu(meshCopy);

    lodMeshes_[worldspaceId] = std::move(meshCopy);
    LOGD_DLOD("Registered LOD mesh '%s' (lod=%d, verts=%lu, indices=%lu)",
              worldspaceId.c_str(), data.lodLevel,
              static_cast<unsigned long>(data.vertices.size()),
              static_cast<unsigned long>(data.indices.size()));
}

LodMeshData DistantLodManager::generateLodFromLand(const std::vector<float>& heightData,
                                                     int32_t cellX, int32_t cellY,
                                                     int lodLevel,
                                                     uint32_t textureId) {
    LodMeshData mesh;
    mesh.lodLevel = lodLevel;
    mesh.cellX = cellX;
    mesh.cellY = cellY;
    mesh.textureId = textureId;

    // Determine downsampled resolution
    int originalSize = 65;  // TERRAIN_RESOLUTION
    int targetSize = std::max(4, originalSize / (config.downsampleFactor * (lodLevel + 1)));

    // Downsample heightmap
    std::vector<float> downsampled;
    if (heightData.size() >= static_cast<size_t>(originalSize * originalSize)) {
        downsampled = downsampleHeightmap(heightData, originalSize, targetSize);
    } else {
        // Use raw data if not standard resolution
        downsampled = heightData;
        targetSize = static_cast<int>(std::sqrt(static_cast<float>(heightData.size())));
        if (targetSize < 2) targetSize = 2;
    }

    // Cell world position
    float cellWorldX = static_cast<float>(cellX) * 128.0f;  // CELL_SIZE
    float cellWorldZ = static_cast<float>(cellY) * 128.0f;
    float cellSize = 128.0f;
    float step = cellSize / static_cast<float>(targetSize - 1);

    // Generate vertices: position (x,y,z), texcoord (u,v), color (r,g,b,a)
    int vertCount = targetSize * targetSize;
    mesh.vertices.reserve(static_cast<size_t>(vertCount) * 8);

    for (int z = 0; z < targetSize; ++z) {
        for (int x = 0; x < targetSize; ++x) {
            float localX = static_cast<float>(x) * step;
            float localZ = static_cast<float>(z) * step;

            // Sample height from downsampled data
            int srcIdx = z * targetSize + x;
            float height = 0.0f;
            if (srcIdx >= 0 && srcIdx < static_cast<int>(downsampled.size())) {
                height = downsampled[static_cast<size_t>(srcIdx)];
            }

            // World position
            float worldX = cellWorldX + localX;
            float worldZ = cellWorldZ + localZ;

            // UV coordinates
            float u = static_cast<float>(x) / static_cast<float>(targetSize - 1);
            float v = static_cast<float>(z) / static_cast<float>(targetSize - 1);

            // Vertex color from height
            float r, g, b;
            getTerrainColor(height, r, g, b);

            // Position
            mesh.vertices.push_back(worldX);
            mesh.vertices.push_back(height);
            mesh.vertices.push_back(worldZ);

            // TexCoord
            mesh.vertices.push_back(u);
            mesh.vertices.push_back(v);

            // Color (with full alpha)
            mesh.vertices.push_back(r);
            mesh.vertices.push_back(g);
            mesh.vertices.push_back(b);
            mesh.vertices.push_back(1.0f);
        }
    }

    // Generate indices (two triangles per quad)
    mesh.indices.reserve(static_cast<size_t>((targetSize - 1) * (targetSize - 1) * 6));
    for (int z = 0; z < targetSize - 1; ++z) {
        for (int x = 0; x < targetSize - 1; ++x) {
            int topLeft = z * targetSize + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * targetSize + x;
            int bottomRight = bottomLeft + 1;

            // Triangle 1
            mesh.indices.push_back(static_cast<uint16_t>(topLeft));
            mesh.indices.push_back(static_cast<uint16_t>(bottomLeft));
            mesh.indices.push_back(static_cast<uint16_t>(topRight));

            // Triangle 2
            mesh.indices.push_back(static_cast<uint16_t>(topRight));
            mesh.indices.push_back(static_cast<uint16_t>(bottomLeft));
            mesh.indices.push_back(static_cast<uint16_t>(bottomRight));
        }
    }

    calculateBoundingSphere(mesh);

    LOGD_DLOD("Generated LOD mesh for cell(%d,%d) lod=%d: %dx%d grid, %lu verts, %lu indices",
              cellX, cellY, lodLevel, targetSize, targetSize,
              static_cast<unsigned long>(mesh.vertices.size() / 8),
              static_cast<unsigned long>(mesh.indices.size()));

    return mesh;
}

void DistantLodManager::unregisterLodMesh(const std::string& worldspaceId) {
    auto it = lodMeshes_.find(worldspaceId);
    if (it != lodMeshes_.end()) {
        releaseMeshGpu(it->second);
        lodMeshes_.erase(it);
        LOGD_DLOD("Unregistered LOD mesh '%s'", worldspaceId.c_str());
    }
}

void DistantLodManager::clearAllMeshes() {
    for (auto& pair : lodMeshes_) {
        releaseMeshGpu(pair.second);
    }
    lodMeshes_.clear();
    LOGI_DLOD("All LOD meshes cleared");
}

// ============================================================================
// Visibility & Rendering
// ============================================================================

std::vector<const LodMeshData*> DistantLodManager::getVisibleLodMeshes(
        const glm::vec3& cameraPos, float viewDistance) const {
    std::vector<const LodMeshData*> visible;
    visible.reserve(lodMeshes_.size());

    stats.totalMeshes = static_cast<uint32_t>(lodMeshes_.size());
    stats.visibleMeshes = 0;
    stats.culledMeshes = 0;
    stats.nearestDistance = viewDistance;
    stats.farthestDistance = 0.0f;

    for (const auto& pair : lodMeshes_) {
        const LodMeshData& mesh = pair.second;

        // Distance check
        float dx = mesh.center.x - cameraPos.x;
        float dz = mesh.center.z - cameraPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        if (dist > viewDistance) {
            stats.culledMeshes++;
            continue;
        }

        // Frustum culling (bounding sphere test)
        if (frustumValid_ && !isSphereInFrustum(mesh.center, mesh.radius)) {
            stats.culledMeshes++;
            continue;
        }

        visible.push_back(&mesh);

        if (dist < stats.nearestDistance) stats.nearestDistance = dist;
        if (dist > stats.farthestDistance) stats.farthestDistance = dist;
    }

    stats.visibleMeshes = static_cast<uint32_t>(visible.size());
    return visible;
}

void DistantLodManager::updateFrustum(const glm::mat4& viewProj) {
    extractFrustumPlanes(viewProj);
    frustumValid_ = true;
}

void DistantLodManager::update(float deltaTime) {
    (void)deltaTime;
    // Future: streaming LOD updates, dynamic quality adjustment
}

void DistantLodManager::render(Renderer* renderer, const glm::mat4& viewProj) {
    (void)renderer;
    if (!initialized_ || !shaderCompiled_) return;

    // Get camera position from world manager
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    if (worldManager_) {
        cameraPos = worldManager_->getCameraPosition();
    }

    // Update frustum
    updateFrustum(viewProj);

    // Get visible meshes
    auto visibleMeshes = getVisibleLodMeshes(cameraPos, config.maxDistance);

    if (visibleMeshes.empty() && !horizonGenerated_) return;

    // Use LOD shader
    glUseProgram(shaderProgram_);

    // Set common uniforms
    if (locViewProj_ >= 0) {
        glUniformMatrix4fv(locViewProj_, 1, GL_FALSE,
                           reinterpret_cast<const float*>(&viewProj));
    }
    if (locCameraPos_ >= 0) {
        glUniform3f(locCameraPos_, cameraPos.x, cameraPos.y, cameraPos.z);
    }
    if (locFadeParams_ >= 0) {
        glUniform4f(locFadeParams_, config.fadeStartDistance, config.fadeEndDistance,
                    0.0f, 0.0f);
    }
    if (locFogParams_ >= 0) {
        glUniform4f(locFogParams_, config.fogDensity, config.fogStart,
                    0.55f, 0.65f);  // Fog color (sky blue-gray)
    }

    // Identity model matrix for world-space meshes
    glm::mat4 identity;
    if (locModel_ >= 0) {
        glUniformMatrix4fv(locModel_, 1, GL_FALSE,
                           reinterpret_cast<const float*>(&identity));
    }

    // Enable blending for distance fade
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render each visible LOD mesh
    for (const LodMeshData* meshPtr : visibleMeshes) {
        if (!meshPtr || !meshPtr->gpuUploaded) continue;

        const LodMeshData& mesh = *meshPtr;

        // Calculate distance-based alpha
        float dx = mesh.center.x - cameraPos.x;
        float dz = mesh.center.z - cameraPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        float alpha = 1.0f - std::max(0.0f, std::min(1.0f,
            (dist - config.fadeStartDistance) /
            std::max(1.0f, config.fadeEndDistance - config.fadeStartDistance)));

        if (locAlpha_ >= 0) {
            glUniform1f(locAlpha_, alpha);
        }

        // Bind texture if available
        if (mesh.textureId && locTexture_ >= 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureId);
            glUniform1i(locTexture_, 0);
        }

        // Draw mesh
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()),
                       GL_UNSIGNED_SHORT, nullptr);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);

    // Render horizon ring
    renderHorizonRing(viewProj, cameraPos);
}

// ============================================================================
// Horizon Ring
// ============================================================================

bool DistantLodManager::generateHorizonRing() {
    HorizonRing ring;
    ring.setRingRadius(config.horizonDistance);
    ring.setBaseHeight(-50.0f);
    ring.setPeakHeight(200.0f);

    if (!ring.generate(config.horizonDistance, 64, 4)) {
        LOGE_DLOD("Failed to generate horizon ring geometry");
        return false;
    }

    // Copy mesh data to horizon data struct
    horizonData_.vertices = ring.getVertexCount() > 0 ? std::vector<float>() : std::vector<float>();
    horizonData_.indices.clear();

    // Generate directly into horizon data
    // Re-generate using the ring object's data
    // We need to regenerate since HorizonRing stores data internally
    // Let's use a simpler approach: generate directly
    horizonData_.ringRadius = config.horizonDistance;
    horizonData_.baseHeight = -50.0f;
    horizonData_.peakHeight = 200.0f;
    horizonData_.segmentCount = 64;
    horizonData_.ringCount = 4;

    // Generate horizon mesh directly into horizonData_
    int segCount = horizonData_.segmentCount;
    int ringCount = horizonData_.ringCount;
    float radius = horizonData_.ringRadius;

    horizonData_.vertices.clear();
    horizonData_.indices.clear();

    // Mountain height presets (simplified 8-direction)
    float mountainHeights[8] = {250.0f, 150.0f, 180.0f, 120.0f, 100.0f, 220.0f, 170.0f, 230.0f};
    float mountainColors[8][3] = {
        {0.22f, 0.20f, 0.16f}, {0.24f, 0.22f, 0.17f},
        {0.23f, 0.21f, 0.17f}, {0.25f, 0.23f, 0.18f},
        {0.26f, 0.24f, 0.19f}, {0.21f, 0.19f, 0.15f},
        {0.24f, 0.22f, 0.17f}, {0.20f, 0.18f, 0.14f}
    };

    for (int ring = 0; ring < ringCount; ++ring) {
        float ringT = static_cast<float>(ring) / static_cast<float>(ringCount - 1);

        for (int seg = 0; seg <= segCount; ++seg) {
            float angle = static_cast<float>(seg) / static_cast<float>(segCount) * 2.0f * 3.14159265f;

            // Direction index (0-7)
            float normAngle = angle / (2.0f * 3.14159265f);
            if (normAngle < 0.0f) normAngle += 1.0f;
            float scaled = normAngle * 8.0f;
            int dir0 = static_cast<int>(scaled) % 8;
            int dir1 = (dir0 + 1) % 8;
            float dirT = scaled - static_cast<float>(static_cast<int>(scaled));

            // Interpolated mountain height
            float peakH = mountainHeights[dir0] + (mountainHeights[dir1] - mountainHeights[dir0]) * dirT;
            float baseH = horizonData_.baseHeight;

            // Ridge modulation
            float ridge = std::sin(angle * 2.5f) * 0.3f + std::sin(angle * 5.3f + 1.5f) * 0.12f;
            float height = baseH + (peakH - baseH) * ringT * (0.5f + ridge * 0.3f + 0.2f);

            // Position
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            float y = height;

            // Color interpolation
            float cr = mountainColors[dir0][0] + (mountainColors[dir1][0] - mountainColors[dir0][0]) * dirT;
            float cg = mountainColors[dir0][1] + (mountainColors[dir1][1] - mountainColors[dir0][1]) * dirT;
            float cb = mountainColors[dir0][2] + (mountainColors[dir1][2] - mountainColors[dir0][2]) * dirT;

            // Alpha: transparent at top ring for sky blending
            float alpha = (ring == ringCount - 1) ? 0.7f : 1.0f;

            // Vertex: x, y, z, r, g, b, a
            horizonData_.vertices.push_back(x);
            horizonData_.vertices.push_back(y);
            horizonData_.vertices.push_back(z);
            horizonData_.vertices.push_back(cr);
            horizonData_.vertices.push_back(cg);
            horizonData_.vertices.push_back(cb);
            horizonData_.vertices.push_back(alpha);
        }
    }

    // Generate indices
    int vertsPerRing = segCount + 1;
    for (int ring = 0; ring < ringCount - 1; ++ring) {
        for (int seg = 0; seg < segCount; ++seg) {
            int tl = ring * vertsPerRing + seg;
            int tr = tl + 1;
            int bl = (ring + 1) * vertsPerRing + seg;
            int br = bl + 1;

            horizonData_.indices.push_back(static_cast<uint16_t>(tl));
            horizonData_.indices.push_back(static_cast<uint16_t>(bl));
            horizonData_.indices.push_back(static_cast<uint16_t>(tr));

            horizonData_.indices.push_back(static_cast<uint16_t>(tr));
            horizonData_.indices.push_back(static_cast<uint16_t>(bl));
            horizonData_.indices.push_back(static_cast<uint16_t>(br));
        }
    }

    // Upload to GPU
    uploadHorizonToGpu();

    horizonGenerated_ = true;
    stats.horizonSegments = static_cast<uint32_t>(segCount);
    LOGI_DLOD("Horizon ring generated: %lu vertices, %lu indices",
              static_cast<unsigned long>(horizonData_.vertices.size() / 7),
              static_cast<unsigned long>(horizonData_.indices.size()));
    return true;
}

void DistantLodManager::renderHorizonRing(const glm::mat4& viewProj,
                                            const glm::vec3& cameraPos) {
    if (!horizonGenerated_ || !horizonData_.gpuUploaded) return;

    // Use horizon shader (same program, different uniform setup)
    // We reuse the same shader program but with identity model matrix
    glUseProgram(shaderProgram_);

    if (locViewProj_ >= 0) {
        glUniformMatrix4fv(locViewProj_, 1, GL_FALSE,
                           reinterpret_cast<const float*>(&viewProj));
    }
    if (locCameraPos_ >= 0) {
        glUniform3f(locCameraPos_, cameraPos.x, cameraPos.y, cameraPos.z);
    }
    if (locAlpha_ >= 0) {
        glUniform1f(locAlpha_, 1.0f);
    }

    // Identity model matrix
    glm::mat4 identity;
    if (locModel_ >= 0) {
        glUniformMatrix4fv(locModel_, 1, GL_FALSE,
                           reinterpret_cast<const float*>(&identity));
    }

    // Disable texture for horizon
    if (locTexture_ >= 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBindVertexArray(horizonData_.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(horizonData_.indices.size()),
                   GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ============================================================================
// Frustum Culling
// ============================================================================

void DistantLodManager::extractFrustumPlanes(const glm::mat4& m) {
    // Left plane
    frustumPlanes_[0].normal.x = m[0][3] + m[0][0];
    frustumPlanes_[0].normal.y = m[1][3] + m[1][0];
    frustumPlanes_[0].normal.z = m[2][3] + m[2][0];
    frustumPlanes_[0].distance = m[3][3] + m[3][0];

    // Right plane
    frustumPlanes_[1].normal.x = m[0][3] - m[0][0];
    frustumPlanes_[1].normal.y = m[1][3] - m[1][0];
    frustumPlanes_[1].normal.z = m[2][3] - m[2][0];
    frustumPlanes_[1].distance = m[3][3] - m[3][0];

    // Bottom plane
    frustumPlanes_[2].normal.x = m[0][3] + m[0][1];
    frustumPlanes_[2].normal.y = m[1][3] + m[1][1];
    frustumPlanes_[2].normal.z = m[2][3] + m[2][1];
    frustumPlanes_[2].distance = m[3][3] + m[3][1];

    // Top plane
    frustumPlanes_[3].normal.x = m[0][3] - m[0][1];
    frustumPlanes_[3].normal.y = m[1][3] - m[1][1];
    frustumPlanes_[3].normal.z = m[2][3] - m[2][1];
    frustumPlanes_[3].distance = m[3][3] - m[3][1];

    // Near plane
    frustumPlanes_[4].normal.x = m[0][3] + m[0][2];
    frustumPlanes_[4].normal.y = m[1][3] + m[1][2];
    frustumPlanes_[4].normal.z = m[2][3] + m[2][2];
    frustumPlanes_[4].distance = m[3][3] + m[3][2];

    // Far plane
    frustumPlanes_[5].normal.x = m[0][3] - m[0][2];
    frustumPlanes_[5].normal.y = m[1][3] - m[1][2];
    frustumPlanes_[5].normal.z = m[2][3] - m[2][2];
    frustumPlanes_[5].distance = m[3][3] - m[3][2];

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        normalizeFrustumPlane(frustumPlanes_[i]);
    }
}

void DistantLodManager::normalizeFrustumPlane(DistantFrustumPlane& plane) {
    float len = std::sqrt(plane.normal.x * plane.normal.x +
                          plane.normal.y * plane.normal.y +
                          plane.normal.z * plane.normal.z);
    if (len > 1e-6f) {
        float invLen = 1.0f / len;
        plane.normal.x *= invLen;
        plane.normal.y *= invLen;
        plane.normal.z *= invLen;
        plane.distance *= invLen;
    }
}

bool DistantLodManager::isSphereInFrustum(const glm::vec3& center, float radius) const {
    for (int i = 0; i < 6; ++i) {
        float dist = frustumPlanes_[i].normal.x * center.x +
                     frustumPlanes_[i].normal.y * center.y +
                     frustumPlanes_[i].normal.z * center.z +
                     frustumPlanes_[i].distance;
        if (dist < -radius) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// GPU Resource Management
// ============================================================================

void DistantLodManager::uploadMeshToGpu(LodMeshData& mesh) {
    if (mesh.vertices.empty() || mesh.indices.empty()) return;

    // Create VAO
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(float)),
                 mesh.vertices.data(), GL_STATIC_DRAW);

    // Position (location 0): 3 floats at offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // TexCoord (location 1): 2 floats at offset 3
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Color (location 2): 4 floats at offset 5
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          reinterpret_cast<void*>(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Create IBO
    glGenBuffers(1, &mesh.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint16_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mesh.gpuUploaded = true;
}

void DistantLodManager::uploadHorizonToGpu() {
    if (horizonData_.vertices.empty() || horizonData_.indices.empty()) return;

    releaseHorizonGpu();

    glGenVertexArrays(1, &horizonData_.vao);
    glBindVertexArray(horizonData_.vao);

    glGenBuffers(1, &horizonData_.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, horizonData_.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(horizonData_.vertices.size() * sizeof(float)),
                 horizonData_.vertices.data(), GL_STATIC_DRAW);

    // Position (location 0): 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Color (location 2): 4 floats (reuse location 2 for compatibility with LOD shader)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &horizonData_.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, horizonData_.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(horizonData_.indices.size() * sizeof(uint16_t)),
                 horizonData_.indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    horizonData_.gpuUploaded = true;
}

void DistantLodManager::releaseMeshGpu(LodMeshData& mesh) {
    if (mesh.vao) { glDeleteVertexArrays(1, &mesh.vao); mesh.vao = 0; }
    if (mesh.vbo) { glDeleteBuffers(1, &mesh.vbo); mesh.vbo = 0; }
    if (mesh.ibo) { glDeleteBuffers(1, &mesh.ibo); mesh.ibo = 0; }
    mesh.gpuUploaded = false;
}

void DistantLodManager::releaseHorizonGpu() {
    if (horizonData_.vao) { glDeleteVertexArrays(1, &horizonData_.vao); horizonData_.vao = 0; }
    if (horizonData_.vbo) { glDeleteBuffers(1, &horizonData_.vbo); horizonData_.vbo = 0; }
    if (horizonData_.ibo) { glDeleteBuffers(1, &horizonData_.ibo); horizonData_.ibo = 0; }
    horizonData_.gpuUploaded = false;
}

// ============================================================================
// Shader Compilation
// ============================================================================

bool DistantLodManager::compileShader() {
    // Compile vertex shader
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertSrc = DISTANT_LOD_VERTEX_SHADER;
    glShaderSource(vertShader, 1, &vertSrc, nullptr);
    glCompileShader(vertShader);

    GLint compiled = 0;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(vertShader, sizeof(log), nullptr, log);
        LOGE_DLOD("Vertex shader compile error: %s", log);
        glDeleteShader(vertShader);
        return false;
    }

    // Compile fragment shader
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragSrc = DISTANT_LOD_FRAGMENT_SHADER;
    glShaderSource(fragShader, 1, &fragSrc, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(fragShader, sizeof(log), nullptr, log);
        LOGE_DLOD("Fragment shader compile error: %s", log);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return false;
    }

    // Link program
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertShader);
    glAttachShader(shaderProgram_, fragShader);
    glLinkProgram(shaderProgram_);

    GLint linked = 0;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(shaderProgram_, sizeof(log), nullptr, log);
        LOGE_DLOD("Shader link error: %s", log);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
        return false;
    }

    // Clean up individual shaders
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    // Cache uniform locations
    locViewProj_ = glGetUniformLocation(shaderProgram_, "uViewProj");
    locModel_ = glGetUniformLocation(shaderProgram_, "uModel");
    locTexture_ = glGetUniformLocation(shaderProgram_, "uTexture");
    locFadeParams_ = glGetUniformLocation(shaderProgram_, "uFadeParams");
    locFogParams_ = glGetUniformLocation(shaderProgram_, "uFogParams");
    locCameraPos_ = glGetUniformLocation(shaderProgram_, "uCameraPos");
    locAlpha_ = glGetUniformLocation(shaderProgram_, "uAlpha");

    shaderCompiled_ = true;
    LOGI_DLOD("Distant LOD shader compiled and linked successfully");
    return true;
}

void DistantLodManager::deleteShader() {
    if (shaderProgram_) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }
    shaderCompiled_ = false;
    locViewProj_ = -1;
    locModel_ = -1;
    locTexture_ = -1;
    locFadeParams_ = -1;
    locFogParams_ = -1;
    locCameraPos_ = -1;
    locAlpha_ = -1;
}

// ============================================================================
// Heightmap Downsampling
// ============================================================================

std::vector<float> DistantLodManager::downsampleHeightmap(
        const std::vector<float>& heightData, int originalSize, int targetSize) const {
    std::vector<float> result(static_cast<size_t>(targetSize * targetSize), 0.0f);

    if (originalSize <= 0 || targetSize <= 0) return result;

    float scale = static_cast<float>(originalSize - 1) / static_cast<float>(targetSize - 1);

    for (int z = 0; z < targetSize; ++z) {
        for (int x = 0; x < targetSize; ++x) {
            float srcX = static_cast<float>(x) * scale;
            float srcZ = static_cast<float>(z) * scale;

            // Bilinear interpolation
            int x0 = static_cast<int>(srcX);
            int z0 = static_cast<int>(srcZ);
            int x1 = std::min(x0 + 1, originalSize - 1);
            int z1 = std::min(z0 + 1, originalSize - 1);

            float fx = srcX - static_cast<float>(x0);
            float fz = srcZ - static_cast<float>(z0);

            float h00 = heightData[static_cast<size_t>(z0 * originalSize + x0)];
            float h10 = heightData[static_cast<size_t>(z0 * originalSize + x1)];
            float h01 = heightData[static_cast<size_t>(z1 * originalSize + x0)];
            float h11 = heightData[static_cast<size_t>(z1 * originalSize + x1)];

            float h = h00 * (1.0f - fx) * (1.0f - fz) +
                      h10 * fx * (1.0f - fz) +
                      h01 * (1.0f - fx) * fz +
                      h11 * fx * fz;

            result[static_cast<size_t>(z * targetSize + x)] = h;
        }
    }

    return result;
}

// ============================================================================
// Terrain Color from Height
// ============================================================================

void DistantLodManager::getTerrainColor(float height, float& r, float& g, float& b) const {
    // Simple height-based coloring
    // Low: dark green/brown, Mid: green/brown, High: gray/white
    if (height < 0.0f) {
        // Water level - dark blue-green
        r = 0.15f; g = 0.20f; b = 0.18f;
    } else if (height < 50.0f) {
        // Lowlands - green
        float t = height / 50.0f;
        r = 0.20f + t * 0.10f;
        g = 0.30f + t * 0.05f;
        b = 0.15f + t * 0.05f;
    } else if (height < 150.0f) {
        // Hills - brown/green
        float t = (height - 50.0f) / 100.0f;
        r = 0.30f + t * 0.15f;
        g = 0.35f - t * 0.10f;
        b = 0.20f + t * 0.05f;
    } else if (height < 300.0f) {
        // Mountains - gray rock
        float t = (height - 150.0f) / 150.0f;
        r = 0.45f + t * 0.10f;
        g = 0.25f + t * 0.15f;
        b = 0.25f + t * 0.15f;
    } else {
        // Snow caps - white
        float t = std::min(1.0f, (height - 300.0f) / 100.0f);
        r = 0.55f + t * 0.35f;
        g = 0.40f + t * 0.40f;
        b = 0.40f + t * 0.40f;
    }
}

// ============================================================================
// Bounding Sphere Calculation
// ============================================================================

void DistantLodManager::calculateBoundingSphere(LodMeshData& mesh) const {
    if (mesh.vertices.empty()) return;

    // Find centroid
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    int vertCount = static_cast<int>(mesh.vertices.size()) / 8;
    if (vertCount == 0) return;

    for (int i = 0; i < vertCount; ++i) {
        cx += mesh.vertices[static_cast<size_t>(i * 8 + 0)];
        cy += mesh.vertices[static_cast<size_t>(i * 8 + 1)];
        cz += mesh.vertices[static_cast<size_t>(i * 8 + 2)];
    }
    cx /= static_cast<float>(vertCount);
    cy /= static_cast<float>(vertCount);
    cz /= static_cast<float>(vertCount);

    mesh.center = glm::vec3(cx, cy, cz);

    // Find max distance from centroid
    float maxDistSq = 0.0f;
    for (int i = 0; i < vertCount; ++i) {
        float dx = mesh.vertices[static_cast<size_t>(i * 8 + 0)] - cx;
        float dy = mesh.vertices[static_cast<size_t>(i * 8 + 1)] - cy;
        float dz = mesh.vertices[static_cast<size_t>(i * 8 + 2)] - cz;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > maxDistSq) maxDistSq = distSq;
    }

    mesh.radius = std::sqrt(maxDistSq);
}

// ============================================================================
// Statistics
// ============================================================================

void DistantLodManager::resetStats() {
    stats = DistantLodStats();
}

size_t DistantLodManager::getGpuMemoryUsage() const {
    size_t total = 0;
    for (const auto& pair : lodMeshes_) {
        const LodMeshData& mesh = pair.second;
        if (mesh.gpuUploaded) {
            total += mesh.vertices.size() * sizeof(float);
            total += mesh.indices.size() * sizeof(uint16_t);
        }
    }
    if (horizonData_.gpuUploaded) {
        total += horizonData_.vertices.size() * sizeof(float);
        total += horizonData_.indices.size() * sizeof(uint16_t);
    }
    return total;
}

size_t DistantLodManager::getCpuMemoryUsage() const {
    size_t total = 0;
    for (const auto& pair : lodMeshes_) {
        const LodMeshData& mesh = pair.second;
        total += mesh.vertices.capacity() * sizeof(float);
        total += mesh.indices.capacity() * sizeof(uint16_t);
    }
    total += horizonData_.vertices.capacity() * sizeof(float);
    total += horizonData_.indices.capacity() * sizeof(uint16_t);
    return total;
}
