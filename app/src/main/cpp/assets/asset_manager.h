#pragma once

#include "nif_parser.h"
#include "dds_loader.h"
#include "../geometry/mesh.h"
#include "../geometry/material.h"
#include <memory>
#include <unordered_map>
#include <string>

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    // Manager lifecycle
    bool initialize();
    void update(float deltaTime);
    void cleanup();

    // Asset loading
    std::shared_ptr<Mesh> loadNifMesh(const std::string& nifPath);
    std::shared_ptr<Material> loadDDSTexture(const std::string& ddsPath);

    // Cache management
    bool cacheExists(const std::string& key) const;
    void cacheEvict(const std::string& key);
    size_t getCacheSize() const;
    void setCacheLimit(size_t bytes);

    // Getters
    std::shared_ptr<NIFParser> getNifParser() { return nifParser; }
    std::shared_ptr<DDSLoader> getDdsLoader() { return ddsLoader; }

private:
    // Parsers
    std::shared_ptr<NIFParser> nifParser;
    std::shared_ptr<DDSLoader> ddsLoader;

    // Cache
    struct CacheEntry {
        std::shared_ptr<void> asset;
        size_t sizeBytes;
        float lastAccessTime;
    };

    std::unordered_map<std::string, CacheEntry> meshCache;
    std::unordered_map<std::string, CacheEntry> textureCache;

    // Cache management
    size_t maxCacheSize;
    size_t currentCacheSize;
    float cacheCheckInterval;
    float timeSinceLastCheck;

    // Helper methods
    void pruneCache();
    void evictLRU();
};
