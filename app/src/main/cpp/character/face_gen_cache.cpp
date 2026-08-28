#include "face_gen_cache.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <cstring>

namespace facegen {

// ============================================================================
// FaceMemoryPool Implementation
// ============================================================================

FaceMemoryPool::FaceMemoryPool() = default;

FaceMemoryPool::~FaceMemoryPool() {
    reset();
}

void* FaceMemoryPool::allocate(size_t size) {
    if (size == 0) return nullptr;

    // Try to allocate from existing blocks
    for (auto& block : blocks_) {
        if (block->used + size <= block->data.size()) {
            void* ptr = block->data.data() + block->used;
            block->used += size;
            usedBytes_ += size;
            return ptr;
        }
    }

    // Need a new block
    size_t blockSize = std::max(BLOCK_SIZE, size);
    auto newBlock = std::make_unique<Block>();
    newBlock->data.resize(blockSize);
    newBlock->used = size;
    totalAllocated_ += blockSize;
    usedBytes_ += size;

    void* ptr = newBlock->data.data();
    blocks_.push_back(std::move(newBlock));
    return ptr;
}

void FaceMemoryPool::deallocate(void* ptr) {
    // Pool allocator doesn't support individual deallocation
    // Use reset() to free all at once
    (void)ptr;
}

void FaceMemoryPool::reset() {
    blocks_.clear();
    totalAllocated_ = 0;
    usedBytes_ = 0;
}

// ============================================================================
// FaceGenCache Implementation
// ============================================================================

FaceGenCache::FaceGenCache() = default;

FaceGenCache::~FaceGenCache() {
    cleanup();
}

bool FaceGenCache::initialize() {
    LOGI_FGCACHE("FaceGenCache initialized (mesh: %lu MB, texture: %lu MB, max faces: %d)",
                 static_cast<unsigned long>(maxMeshCacheSize_ / (1024 * 1024)),
                 static_cast<unsigned long>(maxTextureCacheSize_ / (1024 * 1024)),
                 maxCachedFaces_);
    return true;
}

void FaceGenCache::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Cleanup GPU resources
    for (auto& [id, mesh] : meshCache_) {
        if (mesh) cleanupMeshGPU(*mesh);
    }
    for (auto& [id, tex] : textureCache_) {
        if (tex) cleanupTextureGPU(*tex);
    }

    meshCache_.clear();
    meshLRUList_.clear();
    textureCache_.clear();
    textureLRUList_.clear();
    currentMeshCacheSize_ = 0;
    currentTextureCacheSize_ = 0;

    memoryPool_.reset();

    LOGI_FGCACHE("FaceGenCache cleaned up");
}

// ============================================================================
// Mesh Cache
// ============================================================================

bool FaceGenCache::cacheMesh(uint32_t npcId, const std::vector<MeshVertex>& vertices,
                              const std::vector<uint32_t>& indices) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already cached
    auto it = meshCache_.find(npcId);
    if (it != meshCache_.end()) {
        // Update existing entry
        updateMeshLRU(npcId);
        cacheHits_++;
        return true;
    }

    cacheMisses_++;

    // Calculate memory usage
    size_t meshSize = vertices.size() * sizeof(MeshVertex) + indices.size() * sizeof(uint32_t);

    // Evict if necessary
    while (currentMeshCacheSize_ + meshSize > maxMeshCacheSize_ ||
           static_cast<int>(meshCache_.size()) >= maxCachedFaces_) {
        if (meshCache_.empty()) break;
        evictOldestMesh();
    }

    // Create cached mesh
    auto cachedMesh = std::make_unique<CachedFaceMesh>();
    cachedMesh->npcId = npcId;
    cachedMesh->vertices = vertices;
    cachedMesh->indices = indices;
    cachedMesh->indexCount = static_cast<uint32_t>(indices.size());
    cachedMesh->memoryBytes = meshSize;
    cachedMesh->lastAccessTime = 0.0f;
    cachedMesh->uploadedToGPU = false;

    // Add to LRU
    meshLRUList_.push_front(npcId);
    currentMeshCacheSize_ += meshSize;
    meshCache_[npcId] = std::move(cachedMesh);

    LOGD_FGCACHE("Cached mesh for NPC %u (%lu vertices, %lu indices, %lu bytes)",
                 npcId,
                 static_cast<unsigned long>(vertices.size()),
                 static_cast<unsigned long>(indices.size()),
                 static_cast<unsigned long>(meshSize));

    return true;
}

const CachedFaceMesh* FaceGenCache::getCachedMesh(uint32_t npcId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = meshCache_.find(npcId);
    if (it == meshCache_.end()) {
        cacheMisses_++;
        return nullptr;
    }

    cacheHits_++;
    updateMeshLRU(npcId);
    it->second->lastAccessTime = static_cast<float>(cacheHits_);
    return it->second.get();
}

bool FaceGenCache::uploadMeshToGPU(uint32_t npcId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = meshCache_.find(npcId);
    if (it == meshCache_.end()) return false;

    CachedFaceMesh& mesh = *it->second;
    if (mesh.uploadedToGPU) return true;

    // Create VAO
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(MeshVertex)),
                 mesh.vertices.data(), GL_STATIC_DRAW);

    // Create EBO
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    // Setup vertex attributes
    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, position)));

    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, normal)));

    // TexCoord (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, texCoord)));

    // Bone weights (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, boneWeights)));

    // Bone indices (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_UNSIGNED_BYTE, sizeof(MeshVertex),
                           reinterpret_cast<void*>(offsetof(MeshVertex, boneIndices)));

    glBindVertexArray(0);

    mesh.uploadedToGPU = true;

    LOGD_FGCACHE("Uploaded mesh for NPC %u to GPU (VAO=%u)", npcId, mesh.vao);
    return true;
}

void FaceGenCache::removeMesh(uint32_t npcId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = meshCache_.find(npcId);
    if (it == meshCache_.end()) return;

    cleanupMeshGPU(*it->second);
    currentMeshCacheSize_ -= it->second->memoryBytes;

    // Remove from LRU list
    meshLRUList_.remove(npcId);
    meshCache_.erase(it);
}

// ============================================================================
// Texture Cache
// ============================================================================

bool FaceGenCache::cacheTexture(uint32_t npcId, const uint8_t* rgbaData,
                                 uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!rgbaData || width == 0 || height == 0) return false;

    // Check if already cached
    auto it = textureCache_.find(npcId);
    if (it != textureCache_.end()) {
        updateTextureLRU(npcId);
        cacheHits_++;
        return true;
    }

    cacheMisses_++;

    size_t texSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

    // Evict if necessary
    while (currentTextureCacheSize_ + texSize > maxTextureCacheSize_ ||
           static_cast<int>(textureCache_.size()) >= maxCachedFaces_) {
        if (textureCache_.empty()) break;
        evictOldestTexture();
    }

    // Create cached texture
    auto cachedTex = std::make_unique<CachedFaceTexture>();
    cachedTex->npcId = npcId;
    cachedTex->width = width;
    cachedTex->height = height;
    cachedTex->pixelData.assign(rgbaData, rgbaData + texSize);
    cachedTex->memoryBytes = texSize;
    cachedTex->lastAccessTime = 0.0f;
    cachedTex->glTextureId = 0;

    // Add to LRU
    textureLRUList_.push_front(npcId);
    currentTextureCacheSize_ += texSize;
    textureCache_[npcId] = std::move(cachedTex);

    LOGD_FGCACHE("Cached texture for NPC %u (%ux%u, %lu bytes)",
                 npcId, width, height, static_cast<unsigned long>(texSize));

    return true;
}

const CachedFaceTexture* FaceGenCache::getCachedTexture(uint32_t npcId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = textureCache_.find(npcId);
    if (it == textureCache_.end()) {
        cacheMisses_++;
        return nullptr;
    }

    cacheHits_++;
    updateTextureLRU(npcId);
    it->second->lastAccessTime = static_cast<float>(cacheHits_);
    return it->second.get();
}

bool FaceGenCache::uploadTextureToGPU(uint32_t npcId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = textureCache_.find(npcId);
    if (it == textureCache_.end()) return false;

    CachedFaceTexture& tex = *it->second;
    if (tex.glTextureId != 0) return true;  // Already uploaded

    // Create OpenGL texture
    glGenTextures(1, &tex.glTextureId);
    glBindTexture(GL_TEXTURE_2D, tex.glTextureId);

    // Upload pixel data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(tex.width), static_cast<GLsizei>(tex.height),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.pixelData.data());

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    LOGD_FGCACHE("Uploaded texture for NPC %u to GPU (texId=%u)", npcId, tex.glTextureId);
    return true;
}

void FaceGenCache::removeTexture(uint32_t npcId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = textureCache_.find(npcId);
    if (it == textureCache_.end()) return;

    cleanupTextureGPU(*it->second);
    currentTextureCacheSize_ -= it->second->memoryBytes;

    // Remove from LRU list
    textureLRUList_.remove(npcId);
    textureCache_.erase(it);
}

// ============================================================================
// Cache Management
// ============================================================================

void FaceGenCache::evictLRU(size_t targetBytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t totalSize = currentMeshCacheSize_ + currentTextureCacheSize_;
    while (totalSize > targetBytes) {
        bool evicted = false;

        // Evict oldest mesh if it's older than oldest texture
        if (!meshLRUList_.empty() &&
            (textureLRUList_.empty() ||
             meshCache_[meshLRUList_.back()]->lastAccessTime <=
             textureCache_[textureLRUList_.back()]->lastAccessTime)) {
            evictOldestMesh();
            evicted = true;
        } else if (!textureLRUList_.empty()) {
            evictOldestTexture();
            evicted = true;
        }

        if (!evicted) break;
        totalSize = currentMeshCacheSize_ + currentTextureCacheSize_;
    }
}

void FaceGenCache::clearAll() {
    cleanup();
}

// ============================================================================
// Internal Helpers
// ============================================================================

void FaceGenCache::updateMeshLRU(uint32_t npcId) {
    meshLRUList_.remove(npcId);
    meshLRUList_.push_front(npcId);
}

void FaceGenCache::updateTextureLRU(uint32_t npcId) {
    textureLRUList_.remove(npcId);
    textureLRUList_.push_front(npcId);
}

void FaceGenCache::evictOldestMesh() {
    if (meshLRUList_.empty()) return;

    uint32_t oldestId = meshLRUList_.back();
    meshLRUList_.pop_back();

    auto it = meshCache_.find(oldestId);
    if (it != meshCache_.end()) {
        cleanupMeshGPU(*it->second);
        currentMeshCacheSize_ -= it->second->memoryBytes;
        meshCache_.erase(it);
        LOGD_FGCACHE("Evicted mesh for NPC %u", oldestId);
    }
}

void FaceGenCache::evictOldestTexture() {
    if (textureLRUList_.empty()) return;

    uint32_t oldestId = textureLRUList_.back();
    textureLRUList_.pop_back();

    auto it = textureCache_.find(oldestId);
    if (it != textureCache_.end()) {
        cleanupTextureGPU(*it->second);
        currentTextureCacheSize_ -= it->second->memoryBytes;
        textureCache_.erase(it);
        LOGD_FGCACHE("Evicted texture for NPC %u", oldestId);
    }
}

void FaceGenCache::cleanupMeshGPU(CachedFaceMesh& mesh) {
    if (mesh.vao != 0) {
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = 0;
    }
    if (mesh.vbo != 0) {
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = 0;
    }
    if (mesh.ebo != 0) {
        glDeleteBuffers(1, &mesh.ebo);
        mesh.ebo = 0;
    }
    mesh.uploadedToGPU = false;
}

void FaceGenCache::cleanupTextureGPU(CachedFaceTexture& tex) {
    if (tex.glTextureId != 0) {
        glDeleteTextures(1, &tex.glTextureId);
        tex.glTextureId = 0;
    }
}

} // namespace facegen
