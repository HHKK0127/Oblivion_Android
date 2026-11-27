#pragma once

#include "nif_parser.h"
#include "dds_loader.h"
#include "bsa_reader.h"
#include "esm_reader.h"
#include "plugin_list.h"
#include "../geometry/mesh.h"
#include "../geometry/material.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
    void clearCache();

    // Negative cache for mesh paths that could not be resolved. Without this,
    // every frame re-attempts a full BSA lookup + file open for each missing
    // mesh, which is catastrophic with thousands of NPCs.
    bool isMeshKnownMissing(const std::string& nifPath) const;
    void markMeshMissing(const std::string& nifPath);
    void clearMissingMeshCache();

    // Asset listing (for debug menu)
    std::string getLoadedTextureList() const;
    std::string getLoadedModelList() const;
    std::string getTextureCacheStats() const;
    std::string getModelCacheStats() const;
    std::string getCacheStats() const;
    std::string getMemoryUsage() const;
    std::string getAssetStats() const;
    void reloadAllAssets();

    // ESM game data
    oblivion::ESMManager& getEsmManager() { return m_esmManager; }
    const oblivion::ESMManager& getEsmManager() const { return m_esmManager; }
    bool loadEsm(const std::string& esmPath);
    bool loadEsmFromArchive(const std::string& esmName);
    // Split form of loadEsmFromArchive(). extractEsmFromArchive() reads the plugin
    // bytes on the calling thread, because BSArchive owns a single stream and is not
    // thread safe. parseEsmFromMemory() parses those bytes; that is the CPU bound
    // part, so it may run on a worker thread while the caller keeps rendering.
    bool extractEsmFromArchive(const std::string& esmName, std::vector<uint8_t>& out) const;
    bool parseEsmFromMemory(const std::string& esmName, const std::vector<uint8_t>& data);

        // Plugins.txt support: load the active plugin list from a plugins.txt file.
        // The file format is one plugin filename per line; lines starting with '#' are
        // comments and a leading '*' marks an active plugin. Returns the number of
        // plugins loaded. See assets/plugin_list.h for the full grammar.
        int loadPluginsFile(const std::string& pluginsTxtPath);

        // Load plugins from a vector of filenames (in load order).
        int loadPluginsFromList(const std::vector<std::string>& pluginNames);

        // Write a plugin selection back to plugins.txt so the launcher's Data Files
        // screen survives a restart. Entries are written in the order given, with a
        // '*' in front of every active one. Returns false when the file cannot be
        // opened for writing (for example when the data path is read only).
        bool savePluginsFile(const std::string& pluginsTxtPath,
                             const std::vector<oblivion::PluginListEntry>& entries) const;

        // Scan the data directory for .esp/.esm files and return their filenames in
        // retail load order (Oblivion.esm, then the remaining masters, then the
        // plugins). This is used by the launcher to populate the Data Files list.
        std::vector<std::string> scanDataFiles() const;

    // Getters
    std::shared_ptr<NIFParser> getNifParser() { return nifParser; }
    std::shared_ptr<DDSLoader> getDdsLoader() { return ddsLoader; }
        size_t getArchiveCount() const { return m_archives.size(); }
        BSArchive* getArchive(size_t index) const {
            return (index < m_archives.size()) ? m_archives[index].get() : nullptr;
        }
                // Get the list of loaded archive file paths (for BSA priority management)
                    std::vector<std::string> getArchivePaths() const;

                    // Reorder BSA archives so that archives belonging to later-loaded plugins
                    // are searched first (higher priority). The pluginLoadOrder is the list of
                    // plugin filenames in the order they were loaded (last = highest priority).
                    void reorderArchivesByPluginOrder(const std::vector<std::string>& pluginLoadOrder);

    // Set game data path (for direct file access fallback)
    void setDataPath(const std::string& dataPath) { m_dataPath = dataPath; }
    const std::string& getDataPath() const { return m_dataPath; }

    // True when a loose plugin exists but extractEsmFromArchive() did not read it
    // yet (the copy has not settled, or the file was rejected and is unchanged).
    // Callers must retry instead of reporting "no game data" (#186).
    bool isLooseEsmPending() const { return m_looseEsmPending; }

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

    // Loose-plugin read gate (#186). A plugin copied into files/data is visible
    // long before it is complete; reading it at that moment used to accept a
    // truncated file as game data. The size must stop changing for
    // LOOSE_ESM_STABLE_MS before the bytes are read, and a plugin whose size
    // did not change after a failed parse is not read again until the file is
    // touched (so a corrupt copy does not turn the 1 Hz retry into an I/O loop).
    static constexpr long long LOOSE_ESM_STABLE_MS = 1500;
    mutable long long m_looseEsmSize = -1;
    mutable long long m_looseEsmMtime = 0;
    mutable long long m_looseEsmSeenAtMs = 0;
    // Set while a loose plugin exists but was deliberately not read (still
    // growing, or rejected and unchanged). The loader must treat this as "wait"
    // rather than "no game data", otherwise a copy in progress flashes the
    // missing-data dialog.
    mutable bool m_looseEsmPending = false;
    mutable long long m_lastReadEsmSize = -1;
    mutable long long m_lastReadEsmMtime = 0;
    // Size/mtime of the last loose read the parser rejected. Written from the
    // const extraction path, hence mutable like the settle-wait state above.
    mutable long long m_rejectedEsmSize = -1;
    mutable long long m_rejectedEsmMtime = 0;

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
    std::unordered_set<std::string> missingMeshPaths;
    static constexpr size_t MAX_MISSING_MESH_ENTRIES = 65536;

    // Cache management
    size_t maxCacheSize;
    size_t currentCacheSize;
    float cacheCheckInterval;
    float timeSinceLastCheck;

    // Helper methods
    void pruneCache();
    void evictLRU();
};
