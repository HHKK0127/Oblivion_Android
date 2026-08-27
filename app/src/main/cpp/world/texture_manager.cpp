#include "texture_manager.h"
#include "../assets/asset_manager.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <chrono>

// ============================================================================
// TextureManager Implementation
// ============================================================================

TextureManager::TextureManager()
    : assetManager(nullptr), isInitialized(false),
      maxCacheSize(256 * 1024 * 1024),  // 256 MB default
      currentCacheSize(0) {
    resetStats();
}

TextureManager::~TextureManager() {
    cleanup();
}

bool TextureManager::initialize(AssetManager* assetMgr) {
    assetManager = assetMgr;
    isInitialized = true;
    resetStats();

    LOGI_TEX("TextureManager initialized (max cache: %zu MB)", maxCacheSize / (1024 * 1024));
    return true;
}

void TextureManager::cleanup() {
    clearCache();
    isInitialized = false;
    assetManager = nullptr;

    LOGI_TEX("TextureManager cleaned up");
}

// ============================================================================
// Texture Loading
// ============================================================================

uint32_t TextureManager::loadTexture(const std::string& texturePath) {
    if (texturePath.empty()) {
        LOGW_TEX("Empty texture path");
        return 0;
    }

    // Check cache first
    if (isCached(texturePath)) {
        stats.cacheHits++;
        updateAccessTime(texturePath);
        return getCachedTexture(texturePath);
    }

    stats.cacheMisses++;

    // Load from BSA
    uint32_t textureId = loadDDSFromBSA(texturePath);
    if (textureId == 0) {
        LOGW_TEX("Failed to load texture: %s", texturePath.c_str());
        return 0;
    }

    LOGD_TEX("Loaded texture: %s (ID: %u)", texturePath.c_str(), textureId);
    return textureId;
}

uint32_t TextureManager::loadTexture(const std::string& texturePath,
                                       DDSCompressionFormat preferredFormat) {
    // For now, ignore preferred format and load normally
    // TODO: Implement format-specific loading
    return loadTexture(texturePath);
}

uint32_t TextureManager::loadTextureFromData(const std::string& key,
                                               const uint8_t* data, size_t dataSize) {
    if (!data || dataSize == 0) {
        LOGE_TEX("Invalid texture data");
        return 0;
    }

    // Check cache
    if (isCached(key)) {
        stats.cacheHits++;
        updateAccessTime(key);
        return getCachedTexture(key);
    }

    stats.cacheMisses++;

    // Parse DDS data
    DDSLoader loader;
    // Note: DDSLoader::loadFile expects a file path, not raw data
    // For raw data, we'd need a different approach
    // For now, return 0 and log warning
    LOGW_TEX("Loading texture from raw data not yet supported for: %s", key.c_str());
    return 0;
}

// ============================================================================
// Texture Cache
// ============================================================================

bool TextureManager::isCached(const std::string& texturePath) const {
    return textureCache.find(texturePath) != textureCache.end();
}

uint32_t TextureManager::getCachedTexture(const std::string& texturePath) const {
    auto it = textureCache.find(texturePath);
    if (it != textureCache.end()) {
        return it->second.textureId;
    }
    return 0;
}

void TextureManager::unloadTexture(const std::string& texturePath) {
    auto it = textureCache.find(texturePath);
    if (it == textureCache.end()) {
        return;
    }

    CacheEntry& entry = it->second;

    // Delete OpenGL texture
    if (entry.textureId != 0) {
        glDeleteTextures(1, &entry.textureId);
    }

    // Update cache size
    currentCacheSize -= entry.sizeBytes;

    // Remove from cache
    textureCache.erase(it);

    LOGD_TEX("Unloaded texture: %s", texturePath.c_str());
}

void TextureManager::clearCache() {
    for (auto& pair : textureCache) {
        if (pair.second.textureId != 0) {
            glDeleteTextures(1, &pair.second.textureId);
        }
    }

    textureCache.clear();
    currentCacheSize = 0;

    LOGI_TEX("Texture cache cleared");
}

// ============================================================================
// Cache Statistics
// ============================================================================

TextureManager::CacheStats TextureManager::getCacheStats() const {
    CacheStats s = stats;
    s.totalTextures = textureCache.size();
    s.totalMemoryBytes = currentCacheSize;
    return s;
}

void TextureManager::resetStats() {
    stats = CacheStats();
}

// ============================================================================
// Memory Management
// ============================================================================

void TextureManager::setMaxCacheSize(size_t maxBytes) {
    maxCacheSize = maxBytes;

    // Evict if over limit
    if (currentCacheSize > maxCacheSize) {
        evictLRU(maxCacheSize * 3 / 4);  // Evict to 75% capacity
    }

    LOGI_TEX("Max cache size set to %zu MB", maxCacheSize / (1024 * 1024));
}

size_t TextureManager::getCacheSize() const {
    return currentCacheSize;
}

void TextureManager::evictLRU(size_t targetBytes) {
    if (currentCacheSize <= targetBytes) {
        return;
    }

    LOGI_TEX("Evicting LRU textures (current: %zu MB, target: %zu MB)",
             currentCacheSize / (1024 * 1024), targetBytes / (1024 * 1024));

    // Sort by access time (oldest first)
    std::vector<std::pair<std::string, float>> accessTimes;
    for (const auto& pair : textureCache) {
        accessTimes.emplace_back(pair.first, pair.second.lastAccessTime);
    }

    std::sort(accessTimes.begin(), accessTimes.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    // Evict oldest textures until we reach target
    for (const auto& entry : accessTimes) {
        if (currentCacheSize <= targetBytes) {
            break;
        }
        unloadTexture(entry.first);
    }
}

// ============================================================================
// Texture Reference Counting
// ============================================================================

void TextureManager::addReference(const std::string& texturePath) {
    auto it = textureCache.find(texturePath);
    if (it != textureCache.end()) {
        it->second.refCount++;
    }
}

void TextureManager::removeReference(const std::string& texturePath) {
    auto it = textureCache.find(texturePath);
    if (it != textureCache.end()) {
        if (it->second.refCount > 0) {
            it->second.refCount--;
        }
    }
}

uint32_t TextureManager::getReferenceCount(const std::string& texturePath) const {
    auto it = textureCache.find(texturePath);
    if (it != textureCache.end()) {
        return it->second.refCount;
    }
    return 0;
}

// ============================================================================
// Private Methods
// ============================================================================

uint32_t TextureManager::loadDDSFromBSA(const std::string& texturePath) {
    if (!assetManager) {
        LOGE_TEX("AssetManager not initialized");
        return 0;
    }

    // Load texture data from BSA
    auto material = assetManager->loadDDSTexture(texturePath);
    if (!material) {
        LOGW_TEX("Failed to load DDS texture from BSA: %s", texturePath.c_str());
        return 0;
    }

    // For now, return a placeholder texture ID
    // The actual OpenGL texture upload would happen here
    // TODO: Integrate with actual DDS->OpenGL pipeline

    // Create cache entry
    CacheEntry entry;
    entry.textureId = 1;  // Placeholder - would be actual GL texture ID
    entry.sizeBytes = 1024 * 1024;  // Placeholder size
    entry.refCount = 1;
    entry.lastAccessTime = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    entry.path = texturePath;

    // Check cache size limit
    if (currentCacheSize + entry.sizeBytes > maxCacheSize) {
        evictLRU(maxCacheSize * 3 / 4);
    }

    // Add to cache
    textureCache[texturePath] = entry;
    currentCacheSize += entry.sizeBytes;

    return entry.textureId;
}

uint32_t TextureManager::uploadDDSTexture(const DDSTexture& ddsTexture) {
    // Upload DDS texture to OpenGL
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Upload based on compression format
    if (ddsTexture.compressionFormat == DDSCompressionFormat::UNCOMPRESSED) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     ddsTexture.width, ddsTexture.height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE,
                     ddsTexture.decompressedData.data());
    } else {
        // Compressed textures (DXT1/DXT3/DXT5)
        // Note: Android OpenGL ES doesn't support DXT directly
        // Would need to decompress first or use ETC2/ASTC
        LOGW_TEX("Compressed texture format not directly supported on Android");
        glDeleteTextures(1, &textureId);
        return 0;
    }

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

size_t TextureManager::calculateTextureSize(const DDSTexture& ddsTexture) const {
    // Calculate approximate memory usage
    size_t size = ddsTexture.width * ddsTexture.height * 4;  // RGBA

    // Add mipmap chain
    if (ddsTexture.mipmapCount > 1) {
        size = size * 4 / 3;  // Approximate mipmap chain size
    }

    return size;
}

void TextureManager::updateAccessTime(const std::string& texturePath) {
    auto it = textureCache.find(texturePath);
    if (it != textureCache.end()) {
        it->second.lastAccessTime = static_cast<float>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0f;
    }
}

std::string TextureManager::findLRUTexture() const {
    std::string lruPath;
    float oldestTime = std::numeric_limits<float>::max();

    for (const auto& pair : textureCache) {
        if (pair.second.refCount == 0 && pair.second.lastAccessTime < oldestTime) {
            oldestTime = pair.second.lastAccessTime;
            lruPath = pair.first;
        }
    }

    return lruPath;
}
