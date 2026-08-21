#pragma once

#include "nif_parser.h"
#include "dds_loader.h"
#include "bsa_reader.h"
#include "esm_reader.h"
#include "../geometry/mesh.h"
#include "../geometry/material.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    // Manager lifecycle
    bool initialize();
    void update(float deltaTime);
    void cleanup();

    // BSA Archive Management
    bool loadArchive(const std::string& bsaPath);
    void closeAllArchives();

    // Asset loading from BSA (path is the game path like "meshes\\architecture\\...")
    std::shared_ptr<Mesh> loadNifMesh(const std::string& nifPath);
    std::shared_ptr<Material> loadDDSTexture(const std::string& ddsPath);

    // Load raw file data from BSA archives
    std::vector<uint8_t> loadFileData(const std::string& path);
    bool fileExists(const std::string& path) const;

    // Search BSA archives
    std::vector<std::string> findFiles(const std::string& prefix) const;
    std::vector<std::string> findFilesByExtension(const std::string& ext) const;

    // Cache management
    bool cacheExists(const std::string& key) const;
    void cacheEvict(const std::string& key);
    size_t getCacheSize() const;
    void setCacheLimit(size_t bytes);

    // ESM game data
    oblivion::ESMManager& getEsmManager() { return m_esmManager; }
    const oblivion::ESMManager& getEsmManager() const { return m_esmManager; }
    bool loadEsm(const std::string& esmPath);
    bool loadEsmFromArchive(const std::string& esmName);

    // Getters
    std::shared_ptr<NIFParser> getNifParser() { return nifParser; }
    std::shared_ptr<DDSLoader> getDdsLoader() { return ddsLoader; }
        size_t getArchiveCount() const { return m_archives.size(); }
        BSArchive* getArchive(size_t index) const {
            return (index < m_archives.size()) ? m_archives[index].get() : nullptr;
        }

    // Set game data path (for direct file access fallback)
    void setDataPath(const std::string& dataPath) { m_dataPath = dataPath; }
    const std::string& getDataPath() const { return m_dataPath; }

private:
    // Load NIF from raw byte data (from BSA or direct file)
    std::shared_ptr<Mesh> loadNifFromData(const std::string& path,
                                          const uint8_t* data, size_t dataSize);

    // Parsers
    std::shared_ptr<NIFParser> nifParser;
    std::shared_ptr<DDSLoader> ddsLoader;

    // BSA Archives
    std::vector<std::unique_ptr<BSArchive>> m_archives;
    std::string m_dataPath;

    // ESM Manager
    oblivion::ESMManager m_esmManager;

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
