#pragma once

#include "bsa_reader.h"
#include "dds_loader.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include <android/log.h>

#define LOG_TAG_ASTEX "AssetTextureManager"
#define LOGD_ASTEX(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_ASTEX, __VA_ARGS__)
#define LOGI_ASTEX(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_ASTEX, __VA_ARGS__)
#define LOGW_ASTEX(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_ASTEX, __VA_ARGS__)
#define LOGE_ASTEX(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_ASTEX, __VA_ARGS__)

// ============================================================================
// Asset Texture Manager - BSA texture extraction pipeline
//
// Handles: BSA -> DDS -> decompress -> OpenGL texture
// Features: texture atlas generation, mipmap generation,
//           DXT1/DXT3/DXT5 -> RGBA conversion, LRU cache
// ============================================================================

// Texture atlas configuration
constexpr int ATLAS_SIZE = 2048;           // Atlas texture dimension (pixels)
constexpr int ATLAS_MAX_SUB_TEX_SIZE = 64; // Max sub-texture size to atlas

// Texture format for internal storage
enum class TextureFormat : uint8_t {
    RGBA8 = 0,
    RGB8 = 1,
    DXT1 = 2,
    DXT3 = 3,
    DXT5 = 4
};

// Sub-texture placement in atlas
struct AtlasRegion {
    int atlasIndex = 0;     // Which atlas this region belongs to
    float u0 = 0.0f;       // UV coordinates (normalized)
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    int x = 0;              // Pixel coordinates in atlas
    int y = 0;
    int width = 0;
    int height = 0;
};

// Loaded texture info
struct TextureInfo {
    uint32_t glTextureId = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipmapCount = 0;
    TextureFormat format = TextureFormat::RGBA8;
    size_t memoryBytes = 0;
    bool inAtlas = false;
    AtlasRegion atlasRegion;
};

class AssetTextureManager {
public:
    AssetTextureManager();
    ~AssetTextureManager();

    // Lifecycle
    bool initialize(BSArchive* archive);
    void cleanup();

    // ========================================================================
    // Texture Loading Pipeline (BSA -> DDS -> decompress -> OpenGL)
    // ========================================================================

    // Load texture from BSA archive, returns OpenGL texture ID
    uint32_t loadTexture(const std::string& texturePath);

    // Load texture from raw DDS data in memory
    uint32_t loadTextureFromData(const std::string& key,
                                  const uint8_t* data, size_t dataSize);

    // Get texture info (returns nullptr if not loaded)
    const TextureInfo* getTextureInfo(const std::string& texturePath) const;

    // ========================================================================
    // Texture Atlas Generation
    // ========================================================================

    // Pack small textures into atlases (call after batch loading)
    int generateAtlases();

    // Get atlas OpenGL texture ID by index
    uint32_t getAtlasTextureId(int atlasIndex) const;

    // Get atlas region for a texture (returns nullptr if not atlased)
    const AtlasRegion* getAtlasRegion(const std::string& texturePath) const;

    // Get number of generated atlases
    int getAtlasCount() const { return static_cast<int>(atlases.size()); }

    // ========================================================================
    // Mipmap Generation
    // ========================================================================

    // Generate mipmaps for a loaded texture
    bool generateMipmaps(const std::string& texturePath);

    // Generate mipmaps for all loaded textures
    void generateAllMipmaps();

    // ========================================================================
    // Format Conversion (DXT1/DXT3/DXT5 -> RGBA)
    // ========================================================================

    // Decompress DXT data to RGBA8
    static bool decompressDXT1(const uint8_t* src, uint8_t* dst,
                                int width, int height);
    static bool decompressDXT3(const uint8_t* src, uint8_t* dst,
                                int width, int height);
    static bool decompressDXT5(const uint8_t* src, uint8_t* dst,
                                int width, int height);

    // ========================================================================
    // LRU Cache Management
    // ========================================================================

    // Set maximum cache size in bytes
    void setMaxCacheSize(size_t maxBytes) { maxCacheSize = maxBytes; }

    // Get current cache usage
    size_t getCacheSize() const { return currentCacheSize; }

    // Get max cache size
    size_t getMaxCacheSize() const { return maxCacheSize; }

    // Evict least recently used textures until under target
    void evictLRU(size_t targetBytes);

    // Unload a specific texture
    void unloadTexture(const std::string& texturePath);

    // Clear all cached textures
    void clearCache();

    // Cache statistics
    size_t getCacheHitCount() const { return cacheHits; }
    size_t getCacheMissCount() const { return cacheMisses; }

private:
    // Atlas internal structure
    struct AtlasEntry {
        int x, y;
        int width, height;
        bool used;
    };

    struct Atlas {
        uint32_t glTextureId = 0;
        int size = ATLAS_SIZE;
        std::vector<AtlasEntry> entries;
        std::vector<uint8_t> pixelData;  // RGBA8
        int nextY = 0;                   // Simple row-packing cursor
    };

    // LRU cache node
    using LRUIterator = std::list<std::string>::iterator;

    struct CacheEntry {
        TextureInfo info;
        LRUIterator lruIter;
        float lastAccessTime = 0.0f;
        std::vector<uint8_t> cpuPixelData;  // RGBA8 CPU-side copy for atlas generation
    };

    // BSA archive reference
    BSArchive* archive = nullptr;
    bool initialized = false;

    // Texture cache (LRU)
    std::unordered_map<std::string, CacheEntry> cache;
    std::list<std::string> lruList;           // Front = most recent
    size_t maxCacheSize = 64 * 1024 * 1024;   // 64 MB default
    size_t currentCacheSize = 0;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;

    // Atlas storage
    std::vector<std::unique_ptr<Atlas>> atlases;

    // Internal methods
    uint32_t uploadToGPU(const uint8_t* data, int width, int height,
                          TextureFormat format, bool generateMips);
    uint32_t loadDDSFromBSA(const std::string& texturePath);
    uint32_t loadDDSFromMemory(const uint8_t* data, size_t dataSize,
                                const std::string& key);
    bool decompressDDSToRGBA(const DDSTexture& dds,
                              std::vector<uint8_t>& rgbaOut);
    void updateLRU(const std::string& key);
    void evictIfNeeded();

    // Atlas packing helpers
    bool packIntoAtlas(const std::string& key, const uint8_t* rgbaData,
                        int width, int height, AtlasRegion& regionOut);
    int createNewAtlas();

    // DXT block decode helpers
    static void decodeDXTBlock(const uint8_t* block, uint8_t* outPixels,
                                bool isDXT1);
    static uint32_t expand565(uint16_t color);
    static uint32_t interpolateColor(uint32_t c0, uint32_t c1,
                                       int idx, bool isDXT1);
};
