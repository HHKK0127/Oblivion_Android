#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <GLES3/gl3.h>
#include "nif_parser.h"

// Mesh cache entry with reference counting
struct CacheEntry {
    std::shared_ptr<NifMeshData> meshData;
    size_t memorySize;      // Size of mesh data in bytes
    uint32_t refCount;      // Reference counter
    uint64_t lastAccessTime; // For LRU eviction

    CacheEntry() : memorySize(0), refCount(0), lastAccessTime(0) {}
};

// Texture cache entry
struct TextureCacheEntry {
    GLuint textureId;
    size_t memorySize;      // Estimated GPU memory
    uint32_t refCount;      // Reference counter
    uint64_t lastAccessTime; // For LRU eviction

    TextureCacheEntry() : textureId(0), memorySize(0), refCount(0), lastAccessTime(0) {}
};

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    // Initialize asset manager with base path
    bool initialize(const std::string& basePath);

    // Load a NIF asset by name
    std::shared_ptr<NifMeshData> loadMesh(const std::string& meshName);

    // Load a texture asset by name
    GLuint loadTexture(const std::string& textureName);

    // Load or create a procedural test texture
    GLuint loadTestTexture(int width = 256, int height = 256);

    // Unload a specific mesh (decrements reference count)
    void unloadMesh(const std::string& meshName);

    // Unload a specific texture
    void unloadTexture(const std::string& textureName);

    // Get mesh from cache without loading (returns nullptr if not loaded)
    std::shared_ptr<NifMeshData> getMesh(const std::string& meshName);

    // Get texture from cache without loading (returns 0 if not loaded)
    GLuint getTexture(const std::string& textureName);

    // Clear all cached assets
    void clearCache();

    // Get memory usage statistics
    size_t getTotalMemoryUsage() const { return totalMemoryUsage; }
    size_t getCachedMeshCount() const { return meshCache.size(); }

    // Set maximum cache size (in bytes)
    void setMaxCacheSize(size_t maxSize) { maxCacheSize = maxSize; }

    // Get asset manager status
    const std::string& getLastError() const { return lastError; }

    // Log cache statistics
    void logCacheStats() const;

private:
    NifParser parser;
    std::unordered_map<std::string, CacheEntry> meshCache;
    std::unordered_map<std::string, TextureCacheEntry> textureCache;
    std::string basePath;
    size_t totalMemoryUsage;
    size_t maxCacheSize;
    std::string lastError;

    // Helper functions
    std::string constructMeshPath(const std::string& meshName);
    std::string constructTexturePath(const std::string& textureName);
    size_t calculateMeshMemorySize(const NifMeshData& mesh);
    size_t calculateTextureMemorySize(uint32_t width, uint32_t height);
    void evictLRUMesh();  // Evict least recently used mesh when cache is full
    void evictLRUTexture();  // Evict least recently used texture when cache is full
    GLuint createProceduralTexture(int width, int height);  // Create checkerboard texture
};
