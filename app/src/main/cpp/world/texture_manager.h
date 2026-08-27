#pragma once

#include "../assets/dds_loader.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <android/log.h>

// Forward declarations
class AssetManager;

#define LOG_TAG_TEXMGR "TextureManager"
#define LOGD_TEX(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_TEXMGR, __VA_ARGS__)
#define LOGI_TEX(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_TEXMGR, __VA_ARGS__)
#define LOGW_TEX(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_TEXMGR, __VA_ARGS__)
#define LOGE_TEX(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_TEXMGR, __VA_ARGS__)

// ============================================================================
// Texture Manager - Manages texture loading and caching from BSA archives
// ============================================================================

class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize(AssetManager* assetMgr);
    void cleanup();

    // ========================================================================
    // Texture Loading
    // ========================================================================

    // Load texture from BSA archive (returns OpenGL texture ID)
    uint32_t loadTexture(const std::string& texturePath);

    // Load texture with specific format (DXT1/DXT3/DXT5)
    uint32_t loadTexture(const std::string& texturePath,
                          DDSCompressionFormat preferredFormat);

    // Load texture from raw DDS data
    uint32_t loadTextureFromData(const std::string& key,
                                  const uint8_t* data, size_t dataSize);

    // ========================================================================
    // Texture Cache
    // ========================================================================

    // Check if texture is cached
    bool isCached(const std::string& texturePath) const;

    // Get cached texture ID (returns 0 if not cached)
    uint32_t getCachedTexture(const std::string& texturePath) const;

    // Remove texture from cache
    void unloadTexture(const std::string& texturePath);

    // Clear all cached textures
    void clearCache();

    // ========================================================================
    // Cache Statistics
    // ========================================================================

    struct CacheStats {
        size_t totalTextures = 0;
        size_t totalMemoryBytes = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
    };

    CacheStats getCacheStats() const;
    void resetStats();

    // ========================================================================
    // Memory Management
    // ========================================================================

    // Set maximum cache size in bytes
    void setMaxCacheSize(size_t maxBytes);

    // Get current cache size
    size_t getCacheSize() const;

    // Evict least recently used textures
    void evictLRU(size_t targetBytes);

    // ========================================================================
    // Texture Reference Counting
    // ========================================================================

    // Add reference to texture
    void addReference(const std::string& texturePath);

    // Remove reference from texture
    void removeReference(const std::string& texturePath);

    // Get reference count
    uint32_t getReferenceCount(const std::string& texturePath) const;

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    AssetManager* assetManager;
    bool isInitialized;

    // Texture cache entry
    struct CacheEntry {
        uint32_t textureId = 0;         // OpenGL texture ID
        size_t sizeBytes = 0;           // Memory usage
        uint32_t refCount = 0;          // Reference count
        float lastAccessTime = 0.0f;    // For LRU eviction
        std::string path;               // Original path
    };

    // Cache storage
    std::unordered_map<std::string, CacheEntry> textureCache;

    // Statistics
    CacheStats stats;

    // Memory limits
    size_t maxCacheSize;
    size_t currentCacheSize;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Load DDS texture from BSA
    uint32_t loadDDSFromBSA(const std::string& texturePath);

    // Upload DDS texture to GPU
    uint32_t uploadDDSTexture(const DDSTexture& ddsTexture);

    // Calculate texture memory size
    size_t calculateTextureSize(const DDSTexture& ddsTexture) const;

    // Update access time for LRU
    void updateAccessTime(const std::string& texturePath);

    // Find least recently used texture
    std::string findLRUTexture() const;
};
