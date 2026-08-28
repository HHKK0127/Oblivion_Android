#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include <vector>
#include <android/log.h>

#include "../geometry/mesh.h"
#include "../assets/mesh_loader.h"

// ============================================================================
// Phase 52: FaceGen Cache
//
// LRU cache for generated face meshes and textures.
// Uses memory pool for vertex data to reduce allocation overhead.
// ============================================================================

#define LOG_TAG_FGCACHE "FaceGenCache"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FGCACHE(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FGCACHE, __VA_ARGS__)
#else
#define LOGD_FGCACHE(...) do {} while(0)
#endif
#define LOGI_FGCACHE(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FGCACHE, __VA_ARGS__)
#define LOGW_FGCACHE(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FGCACHE, __VA_ARGS__)
#define LOGE_FGCACHE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FGCACHE, __VA_ARGS__)

namespace facegen {

// ============================================================================
// Cached Face Data
// ============================================================================

struct CachedFaceMesh {
    uint32_t npcId = 0;
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t indexCount = 0;
    bool uploadedToGPU = false;
    size_t memoryBytes = 0;
    float lastAccessTime = 0.0f;
};

struct CachedFaceTexture {
    uint32_t npcId = 0;
    uint32_t glTextureId = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixelData;  // RGBA8
    size_t memoryBytes = 0;
    float lastAccessTime = 0.0f;
};

// ============================================================================
// Memory Pool for Vertex Data
// ============================================================================

class FaceMemoryPool {
public:
    static constexpr size_t BLOCK_SIZE = 256 * 1024;  // 256 KB blocks

    FaceMemoryPool();
    ~FaceMemoryPool();

    void* allocate(size_t size);
    void deallocate(void* ptr);
    void reset();
    size_t getTotalAllocated() const { return totalAllocated_; }
    size_t getUsedBytes() const { return usedBytes_; }

private:
    struct Block {
        std::vector<uint8_t> data;
        size_t used = 0;
    };

    std::vector<std::unique_ptr<Block>> blocks_;
    size_t totalAllocated_ = 0;
    size_t usedBytes_ = 0;
};

// ============================================================================
// FaceGen Cache (LRU)
// ============================================================================

class FaceGenCache {
public:
    // Cache limits
    static constexpr size_t DEFAULT_MAX_MESH_CACHE_SIZE = 32 * 1024 * 1024;   // 32 MB
    static constexpr size_t DEFAULT_MAX_TEXTURE_CACHE_SIZE = 16 * 1024 * 1024; // 16 MB
    static constexpr int DEFAULT_MAX_CACHED_FACES = 64;

    FaceGenCache();
    ~FaceGenCache();

    // Lifecycle
    bool initialize();
    void cleanup();

    // ========================================================================
    // Mesh Cache
    // ========================================================================

    // Store a generated face mesh
    bool cacheMesh(uint32_t npcId, const std::vector<MeshVertex>& vertices,
                   const std::vector<uint32_t>& indices);

    // Get cached face mesh (returns nullptr if not cached)
    const CachedFaceMesh* getCachedMesh(uint32_t npcId);

    // Upload cached mesh to GPU
    bool uploadMeshToGPU(uint32_t npcId);

    // Remove cached mesh
    void removeMesh(uint32_t npcId);

    // ========================================================================
    // Texture Cache
    // ========================================================================

    // Store a blended face texture
    bool cacheTexture(uint32_t npcId, const uint8_t* rgbaData,
                      uint32_t width, uint32_t height);

    // Get cached face texture (returns nullptr if not cached)
    const CachedFaceTexture* getCachedTexture(uint32_t npcId);

    // Upload cached texture to GPU
    bool uploadTextureToGPU(uint32_t npcId);

    // Remove cached texture
    void removeTexture(uint32_t npcId);

    // ========================================================================
    // Cache Management
    // ========================================================================

    // Evict least recently used entries to free memory
    void evictLRU(size_t targetBytes);

    // Clear all cached data
    void clearAll();

    // Set cache limits
    void setMaxMeshCacheSize(size_t bytes) { maxMeshCacheSize_ = bytes; }
    void setMaxTextureCacheSize(size_t bytes) { maxTextureCacheSize_ = bytes; }
    void setMaxCachedFaces(int count) { maxCachedFaces_ = count; }

    // Statistics
    size_t getMeshCacheSize() const { return currentMeshCacheSize_; }
    size_t getTextureCacheSize() const { return currentTextureCacheSize_; }
    int getCachedMeshCount() const { return static_cast<int>(meshCache_.size()); }
    int getCachedTextureCount() const { return static_cast<int>(textureCache_.size()); }
    size_t getCacheHitCount() const { return cacheHits_; }
    size_t getCacheMissCount() const { return cacheMisses_; }
    float getHitRate() const {
        size_t total = cacheHits_ + cacheMisses_;
        return total > 0 ? static_cast<float>(cacheHits_) / static_cast<float>(total) : 0.0f;
    }

    // Memory pool access
    FaceMemoryPool& getMemoryPool() { return memoryPool_; }

private:
    // Mesh cache (LRU)
    using MeshLRUIterator = std::list<uint32_t>::iterator;
    std::unordered_map<uint32_t, std::unique_ptr<CachedFaceMesh>> meshCache_;
    std::list<uint32_t> meshLRUList_;  // Front = most recent
    size_t currentMeshCacheSize_ = 0;
    size_t maxMeshCacheSize_ = DEFAULT_MAX_MESH_CACHE_SIZE;

    // Texture cache (LRU)
    using TexLRUIterator = std::list<uint32_t>::iterator;
    std::unordered_map<uint32_t, std::unique_ptr<CachedFaceTexture>> textureCache_;
    std::list<uint32_t> textureLRUList_;  // Front = most recent
    size_t currentTextureCacheSize_ = 0;
    size_t maxTextureCacheSize_ = DEFAULT_MAX_TEXTURE_CACHE_SIZE;

    // Limits
    int maxCachedFaces_ = DEFAULT_MAX_CACHED_FACES;

    // Statistics
    size_t cacheHits_ = 0;
    size_t cacheMisses_ = 0;

    // Memory pool
    FaceMemoryPool memoryPool_;

    // Thread safety
    mutable std::mutex mutex_;

    // Internal helpers
    void updateMeshLRU(uint32_t npcId);
    void updateTextureLRU(uint32_t npcId);
    void evictOldestMesh();
    void evictOldestTexture();
    void cleanupMeshGPU(CachedFaceMesh& mesh);
    void cleanupTextureGPU(CachedFaceTexture& tex);
};

} // namespace facegen
