#include "asset_manager.h"
#include "asset_path_resolver.h"
#include "mesh_builder.h"
#include "../game/gmst_defaults.h"
#include <android/log.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#define LOG_TAG "AssetManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
long long steadyMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Cheap header gate for loose plugins, applied before the parse (#186).
//
// Every plugin starts with a 20-byte TES4 record header. Oblivion's TES4 body
// starts with the HEDR subrecord, whose first field is the format version
// (1.0). Tool-made plugins can ship an empty TES4 body (the host smoke fixture
// does: `TES4` then `GRUP` at offset 20), so the version is checked only when
// HEDR is present.
bool hasTes4Header(const std::vector<uint8_t>& bytes) {
    constexpr size_t kRecordHeader = 20;
    constexpr size_t kSubHeader = 6;
    if (bytes.size() < kRecordHeader) return false;
    if (std::memcmp(bytes.data(), "TES4", 4) != 0) return false;
    if (bytes.size() < kRecordHeader + kSubHeader ||
        std::memcmp(bytes.data() + kRecordHeader, "HEDR", 4) != 0) {
        return true;   // no HEDR subrecord: nothing more to check here
    }
    uint16_t subSize = 0;
    std::memcpy(&subSize, bytes.data() + kRecordHeader + 4, sizeof(subSize));
    if (subSize < 12 || bytes.size() < kRecordHeader + kSubHeader + 4) return false;
    float version = 0.0f;
    std::memcpy(&version, bytes.data() + kRecordHeader + kSubHeader, sizeof(version));
    if (version < 0.99f || version > 1.01f) {
        LOGD("Loose plugin HEDR version is %.3f (expected 1.0 for Oblivion)", version);
        return false;
    }
    return true;
}
}  // namespace

AssetManager::AssetManager()
    : maxCacheSize(500 * 1024 * 1024),  // 500 MB default
      currentCacheSize(0),
      cacheCheckInterval(30.0f),
      timeSinceLastCheck(0.0f) {
}

AssetManager::~AssetManager() {
    cleanup();
}

bool AssetManager::initialize() {
    LOGD("=== AssetManager initialization ===");

    // Initialize NIF parser
    nifParser = std::make_shared<NIFParser>();
    if (!nifParser) {
        LOGE("Failed to create NIFParser");
        return false;
    }

    // Initialize DDS loader
    ddsLoader = std::make_shared<DDSLoader>();
    if (!ddsLoader) {
        LOGE("Failed to create DDSLoader");
        return false;
    }

    LOGD("AssetManager initialized successfully");
    LOGD("Cache limit: %zu MB", maxCacheSize / (1024 * 1024));

    return true;
}

void AssetManager::update(float deltaTime) {
    timeSinceLastCheck += deltaTime;

    // Update lastAccessTime for all cached entries
    for (auto& entry : meshCache) {
        entry.second.lastAccessTime += deltaTime;
    }
    for (auto& entry : textureCache) {
        entry.second.lastAccessTime += deltaTime;
    }

    // Periodically check cache size
    if (timeSinceLastCheck >= cacheCheckInterval) {
        timeSinceLastCheck = 0.0f;

        if (currentCacheSize > maxCacheSize) {
            LOGD("Cache size (%zu MB) exceeds limit (%zu MB), pruning...",
                 currentCacheSize / (1024 * 1024),
                 maxCacheSize / (1024 * 1024));
            pruneCache();
        }
    }
}

void AssetManager::cleanup() {
    LOGD("Cleaning up AssetManager...");

    meshCache.clear();
    textureCache.clear();
    currentCacheSize = 0;

    closeAllArchives();

    if (ddsLoader) {
        ddsLoader->cleanup();
    }

    LOGD("AssetManager cleanup complete");
}

// ============================================================================
// BSA Archive Management
// ============================================================================

bool AssetManager::loadArchive(const std::string& bsaPath) {
    // Try with dataPath prefix first, then raw path
    std::string fullPath = bsaPath;
    if (!m_dataPath.empty() && bsaPath.find('/') == std::string::npos && bsaPath.find('\\') == std::string::npos) {
        fullPath = m_dataPath + "/" + bsaPath;
    }
    LOGD("Loading BSA archive: %s (full: %s)", bsaPath.c_str(), fullPath.c_str());

    auto archive = std::make_unique<BSArchive>();
    if (!archive->open(fullPath)) {
        LOGE("Failed to open BSA archive: %s", fullPath.c_str());
        return false;
    }

    LOGD("BSA archive loaded: %s (%u files, compressed=%s)",
         bsaPath.c_str(),
         archive->getFileCount(),
         archive->isCompressed() ? "yes" : "no");

    m_archives.push_back(std::move(archive));
    return true;
}

void AssetManager::closeAllArchives() {
    LOGD("Closing %zu BSA archives", m_archives.size());
    m_archives.clear();
}

// ============================================================================
// ESM Plugin Loading
// ============================================================================

// After each successful ESM/plugin parse, publish GMST overrides so gameplay
// code sees the loaded values through gmst::getLive()/getStringLive(). This
// runs on the same thread as the parse (worker or main), and the atomic
// release/acquire pattern in gmst::setOverride()/setStringOverride() makes it
// safe for concurrent readers on the render thread.
//
// Parity note (RE V60): the EXE GMST handler looks the EDID up in the
// registered map and IGNORES a miss - it never inserts - so iterating our
// tracked tables (driver-table style) reproduces the load-time behavior
// exactly. The four ESM-only fRegionGenTex* keys are absent from the EXE and
// intentionally not tracked here.
static void publishGmstOverrides(const oblivion::ESMManager& esm) {
    int nOverride = 0;
    constexpr std::size_t N = oblivion::gmst::kEntryCount;
    for (std::size_t i = 0; i < N; ++i) {
        const char* key = oblivion::gmst::kEntries[i].key;
        const oblivion::GameSettingData* s = esm.findGameSetting(key);
        if (s && s->valueType == 'f') {
            oblivion::gmst::setOverride(key, s->numericValue);
            ++nOverride;
        }
    }
    int nStringOverride = 0;
    constexpr std::size_t NS = oblivion::gmst::kStringEntryCount;
    for (std::size_t i = 0; i < NS; ++i) {
        const char* key = oblivion::gmst::kStringEntries[i].key;
        const oblivion::GameSettingData* s = esm.findGameSetting(key);
        if (s && s->valueType == 's') {
            oblivion::gmst::setStringOverride(key, s->stringValue.c_str());
            ++nStringOverride;
        }
    }
    LOGD("GMST overrides published: %d / %zu (float), %d / %zu (string)",
         nOverride, N, nStringOverride, NS);
}

bool AssetManager::loadEsm(const std::string& esmPath) {
    LOGD("Loading ESM/ESP plugin: %s", esmPath.c_str());
    const bool ok = m_esmManager.loadPlugin(esmPath);
    if (ok) publishGmstOverrides(m_esmManager);
    return ok;
}

bool AssetManager::extractEsmFromArchive(const std::string& esmName,
                                         std::vector<uint8_t>& out) const {
    out.clear();
    m_looseEsmPending = false;

    // The ESM is stored inside a BSA. Find the ESM file from the archives.
    std::string searchPath = esmName;
    std::replace(searchPath.begin(), searchPath.end(), '\\', '/');

    // Search all loaded BSA archives for the ESM file
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        const BSAFileEntry* entry = (*it)->findFile(searchPath);
        if (!entry) {
            continue;
        }

        LOGD("Found ESM in BSA: %s (offset=%u, size=%u)",
             searchPath.c_str(), entry->offset, entry->size);

        if ((*it)->extractFileDecompressed(*entry, out)) {
            // The archive's directory record knows the exact size, so a short
            // extraction is detectable without any timing guesswork (#186).
            // realSize is 0 for some compressed entries; fall back to the
            // parser's tail check in that case.
            const size_t expected = entry->compressed ? entry->realSize : entry->size;
            if (expected != 0 && out.size() != expected) {
                LOGE("ESM extraction is short: %s in BSA gave %zu of %zu bytes",
                     searchPath.c_str(), out.size(), expected);
                out.clear();
                continue;
            }
            LOGD("Extracted ESM data: %zu bytes", out.size());
            return true;
        }
    }

    // Fallback to a plain file on disk (loose plugin next to the BSAs).
    //
    // A loose plugin can be a copy in progress: the file exists and grows for
    // tens of seconds, and the record walk does not fail on a truncated tail,
    // so reading it early accepted 0 records (or a partial world) as game data
    // (#186). Only read bytes whose size and mtime have stopped changing, and
    // sanity-check the TES4 header before handing them to the parser.
    if (!m_dataPath.empty()) {
        const std::string fullPath = m_dataPath + "/" + searchPath;
        struct stat fileStat {};
        if (::stat(fullPath.c_str(), &fileStat) == 0 && fileStat.st_size > 0) {
            const long long size = static_cast<long long>(fileStat.st_size);
            const long long mtime = static_cast<long long>(fileStat.st_mtime);
            const long long nowMs = steadyMillis();

            if (size == m_rejectedEsmSize && mtime == m_rejectedEsmMtime) {
                // Already read once and the parser rejected it; the file has not
                // changed, so re-reading it every second only burns I/O.
                m_looseEsmPending = true;
                LOGW("Loose plugin %s was rejected and is unchanged (%lld bytes)",
                     searchPath.c_str(), size);
                return false;
            }
            if (size != m_looseEsmSize || mtime != m_looseEsmMtime) {
                m_looseEsmSize = size;
                m_looseEsmMtime = mtime;
                m_looseEsmSeenAtMs = nowMs;
                m_looseEsmPending = true;
                LOGI("Loose plugin %s is %lld bytes; waiting %lld ms for it to settle",
                     searchPath.c_str(), size, static_cast<long long>(LOOSE_ESM_STABLE_MS));
                return false;
            }
            if (nowMs - m_looseEsmSeenAtMs < LOOSE_ESM_STABLE_MS) {
                m_looseEsmPending = true;
                return false;
            }

            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                const std::streamsize fileSize = file.tellg();
                file.seekg(0, std::ios::beg);
                if (fileSize > 0) {
                    out.resize(static_cast<size_t>(fileSize));
                    if (file.read(reinterpret_cast<char*>(out.data()), fileSize)) {
                        // The file can still change between stat() and read().
                        if (out.size() != static_cast<size_t>(size)) {
                            LOGE("Loose plugin %s changed while reading (%zu of %lld bytes)",
                                 searchPath.c_str(), out.size(), size);
                            out.clear();
                            m_looseEsmSize = -1;   // re-arm the settle wait
                            m_looseEsmPending = true;
                            return false;
                        }
                        if (!hasTes4Header(out)) {
                            LOGE("Loose plugin %s is not a TES4 plugin (%zu bytes)",
                                 searchPath.c_str(), out.size());
                            out.clear();
                            m_rejectedEsmSize = size;
                            m_rejectedEsmMtime = mtime;
                            return false;
                        }
                        LOGD("Loaded ESM from file: %s (%zu bytes)", fullPath.c_str(), out.size());
                        m_lastReadEsmSize = size;
                        m_lastReadEsmMtime = mtime;
                        return true;
                    }
                    out.clear();
                }
            }
        }
    }

    LOGE("ESM file not found: %s", esmName.c_str());
    return false;
}

bool AssetManager::parseEsmFromMemory(const std::string& esmName,
                                      const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return false;
    }
    const bool ok = m_esmManager.loadPluginFromMemory(esmName, data.data(), data.size());
    if (ok) {
        publishGmstOverrides(m_esmManager);
        m_rejectedEsmSize = -1;
        m_rejectedEsmMtime = 0;
        return true;
    }
    // Remember which loose file produced the invalid bytes so the retry loop
    // does not re-read an unchanged, already-rejected copy every second (#186).
    if (m_lastReadEsmSize > 0 && static_cast<long long>(data.size()) == m_lastReadEsmSize) {
        m_rejectedEsmSize = m_lastReadEsmSize;
        m_rejectedEsmMtime = m_lastReadEsmMtime;
    }
    return false;
}

bool AssetManager::loadEsmFromArchive(const std::string& esmName) {
    LOGD("Loading ESM from BSA archive: %s", esmName.c_str());

    // extractEsmFromArchive() also covers the loose-file fallback.
    std::vector<uint8_t> data;
    if (!extractEsmFromArchive(esmName, data)) {
        return false;
    }
    return parseEsmFromMemory(esmName, data);
}

    int AssetManager::loadPluginsFile(const std::string& pluginsTxtPath) {
        LOGD("Loading plugins.txt: %s", pluginsTxtPath.c_str());
        std::ifstream file(pluginsTxtPath);
        if (!file.is_open()) {
            LOGW("plugins.txt not found at %s, using default Oblivion.esm only",
                 pluginsTxtPath.c_str());
            // Default: just load Oblivion.esm
            if (!m_dataPath.empty()) {
                loadEsm(m_dataPath + "/Oblivion.esm");
            }
            return 0;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        file.close();

        const auto entries = oblivion::parsePluginList(buffer.str());
        const auto pluginNames = oblivion::activePluginNames(entries);
        LOGD("plugins.txt: %zu entries, %zu active", entries.size(), pluginNames.size());
        return loadPluginsFromList(pluginNames);
    }

    bool AssetManager::savePluginsFile(
            const std::string& pluginsTxtPath,
            const std::vector<oblivion::PluginListEntry>& entries) const {
        std::ofstream file(pluginsTxtPath, std::ios::trunc);
        if (!file.is_open()) {
            LOGW("Cannot write plugins.txt at %s (data path read only?)",
                 pluginsTxtPath.c_str());
            return false;
        }
        file << oblivion::serializePluginList(entries);
        const bool ok = file.good();
        file.close();
        if (ok) {
            LOGI("plugins.txt written: %s (%zu entries)", pluginsTxtPath.c_str(),
                 entries.size());
        } else {
            LOGW("Failed to write plugins.txt at %s", pluginsTxtPath.c_str());
        }
        return ok;
    }

    int AssetManager::loadPluginsFromList(const std::vector<std::string>& pluginNames) {
        // Set data path on ESMManager for master resolution
        m_esmManager.setDataPath(m_dataPath);

        int loaded = m_esmManager.loadPluginsFromList(pluginNames);
        if (loaded > 0) {
            publishGmstOverrides(m_esmManager);
        }
        return loaded;
    }

    std::vector<std::string> AssetManager::getArchivePaths() const {
        std::vector<std::string> paths;
        paths.reserve(m_archives.size());
        for (const auto& archive : m_archives) {
            paths.push_back(archive->getFilePath());
        }
        return paths;
    }

        void AssetManager::reorderArchivesByPluginOrder(const std::vector<std::string>& pluginLoadOrder) {
            if (m_archives.size() <= 1) return;

            // Build a set of BSA filenames that correspond to each plugin.
            // Convention: a plugin "SomeMod.esp" has a BSA "SomeMod.bsa".
            std::unordered_set<std::string> pluginBsaNames;
            for (const auto& plugin : pluginLoadOrder) {
                std::string bsaName = plugin;
                size_t dot = bsaName.rfind('.');
                if (dot != std::string::npos) {
                    bsaName = bsaName.substr(0, dot) + ".bsa";
                }
                // Normalise to lowercase for comparison
                std::transform(bsaName.begin(), bsaName.end(), bsaName.begin(), ::tolower);
                pluginBsaNames.insert(bsaName);
            }

            // Stable partition: archives whose BSA name matches a plugin in the load
            // order are moved to the front, preserving their relative order so that
            // later-loaded plugins' BSAs end up earlier in the search order.
            // (loadFileData searches in reverse, so earlier in the vector = higher priority.)
            std::stable_partition(m_archives.begin(), m_archives.end(),
                [&pluginBsaNames](const std::unique_ptr<BSArchive>& archive) {
                    std::string name = archive->getFilePath();
                    size_t sep = name.find_last_of("/\\");
                    if (sep != std::string::npos) name = name.substr(sep + 1);
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    return pluginBsaNames.count(name) > 0;
                });

            LOGD("BSA archives reordered by plugin priority (%zu archives)", m_archives.size());
        }

    std::vector<std::string> AssetManager::scanDataFiles() const {
        std::vector<std::string> files;
        if (m_dataPath.empty()) return files;

        // Use platform-specific directory listing
        // On Android, we scan the assets directory or data path
        DIR* dir = opendir(m_dataPath.c_str());
        if (!dir) {
            LOGW("Cannot scan data directory: %s", m_dataPath.c_str());
            return files;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name(entry->d_name);
            // Skip hidden files and directories
            if (name.empty() || name[0] == '.') continue;

            // Check for .esm and .esp extensions
            size_t dot = name.rfind('.');
            if (dot == std::string::npos) continue;
            std::string ext = name.substr(dot);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".esm" || ext == ".esp") {
                files.push_back(name);
            }
        }
        closedir(dir);

        // Retail load order: Oblivion.esm, then the remaining masters, then the
        // plugins, alphabetical inside each group so the list is stable between runs.
        std::vector<oblivion::PluginListEntry> entries;
        entries.reserve(files.size());
        for (const auto& name : files) {
            entries.push_back({name, true});
        }
        oblivion::sortPluginLoadOrder(entries);

        files.clear();
        files.reserve(entries.size());
        for (const auto& entry : entries) {
            files.push_back(entry.name);
        }

        LOGD("Data files scan: found %zu plugins", files.size());
        return files;
    }

// ============================================================================
// BSA File Lookup
// ============================================================================

std::vector<uint8_t> AssetManager::loadFileData(const std::string& path) {
    std::string searchPath = path;
    std::replace(searchPath.begin(), searchPath.end(), '\\', '/');

    for (const auto& candidate : buildAssetPathCandidates(searchPath)) {
        // Search BSA archives in reverse order (last loaded = highest priority)
        for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
            const BSAFileEntry* entry = (*it)->findFile(candidate);
            if (!entry) {
                continue;
            }
            std::vector<uint8_t> data;
            if ((*it)->extractFileDecompressed(*entry, data)) {
                LOGD("Loaded file from BSA: %s (%zu bytes)", candidate.c_str(), data.size());
                return data;
            }
        }

        // Fallback to direct file access
        if (!m_dataPath.empty()) {
            std::string fullPath = m_dataPath + "/" + candidate;
            std::ifstream file(fullPath, std::ios::binary);
            if (file.is_open()) {
                file.seekg(0, std::ios::end);
                size_t size = file.tellg();
                file.seekg(0, std::ios::beg);
                std::vector<uint8_t> data(size);
                file.read(reinterpret_cast<char*>(data.data()), size);
                LOGD("Loaded file from disk: %s (%zu bytes)", fullPath.c_str(), data.size());
                return data;
            }
        }
    }

    LOGE("File not found: %s", searchPath.c_str());
    return {};
}

bool AssetManager::fileExists(const std::string& path) const {
    std::string searchPath = path;
    std::replace(searchPath.begin(), searchPath.end(), '\\', '/');

    for (const auto& candidate : buildAssetPathCandidates(searchPath)) {
        for (const auto& archive : m_archives) {
            if (archive->findFile(candidate)) {
                return true;
            }
        }

        if (!m_dataPath.empty()) {
            std::ifstream file(m_dataPath + "/" + candidate);
            if (file.good()) {
                return true;
            }
        }
    }

    return false;
}

std::vector<std::string> AssetManager::findFiles(const std::string& prefix) const {
    std::vector<std::string> results;
    std::string searchPrefix = prefix;
    std::replace(searchPrefix.begin(), searchPrefix.end(), '\\', '/');

    for (const auto& archive : m_archives) {
        auto entries = archive->findFilesByPrefix(searchPrefix);
        for (const auto* entry : entries) {
            results.push_back(entry->fullPath);
        }
    }
    return results;
}

std::vector<std::string> AssetManager::findFilesByExtension(const std::string& ext) const {
    std::vector<std::string> results;
    for (const auto& archive : m_archives) {
        auto entries = archive->findFilesByExtension(ext);
        for (const auto* entry : entries) {
            results.push_back(entry->fullPath);
        }
    }
    return results;
}

// ============================================================================
// Asset Loading
// ============================================================================

std::shared_ptr<Mesh> AssetManager::loadNifMesh(const std::string& nifPath) {
    // Negative cache: never retry a path that already failed to resolve.
    if (missingMeshPaths.find(nifPath) != missingMeshPaths.end()) {
        return nullptr;
    }

    // Check cache first
    if (meshCache.find(nifPath) != meshCache.end()) {
        LOGD("Mesh found in cache: %s", nifPath.c_str());
        meshCache[nifPath].lastAccessTime = 0.0f;
        return std::static_pointer_cast<Mesh>(meshCache[nifPath].asset);
    }

    LOGD("Loading NIF mesh: %s", nifPath.c_str());

    // Try loading from BSA archives first
    std::vector<uint8_t> fileData = loadFileData(nifPath);
    if (!fileData.empty()) {
        auto mesh = loadNifFromData(nifPath, fileData.data(), fileData.size());
        if (mesh) {
            CacheEntry entry;
            entry.asset = mesh;
            entry.sizeBytes = sizeof(Mesh) + (mesh->getIndexCount() * sizeof(unsigned int));
            entry.lastAccessTime = 0.0f;
            meshCache[nifPath] = entry;
            currentCacheSize += entry.sizeBytes;
            LOGD("Mesh loaded from BSA: %s", nifPath.c_str());
            return mesh;
        }
    }

    // Fallback: try direct file path (legacy)
    // Normalize path separators for Android
    std::string normalizedPath = nifPath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    if (!nifParser->parseFile(normalizedPath)) {
        LOGE("Failed to parse NIF file: %s", nifPath.c_str());
        markMeshMissing(nifPath);
        return nullptr;
    }

    // Create mesh from NIF geometry
    auto mesh = std::make_shared<Mesh>();

    const auto geometries = nifParser->extractAllGeometry();
    const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);
    if (!merged.vertices.empty() && !merged.indices.empty()) {
        mesh->setVertices(merged.vertices);
        mesh->setIndices(merged.indices);
        mesh->uploadToGPU();

        LOGD("Mesh created: %zu of %zu geometries, %zu vertices, %zu indices",
             merged.geometryCount, geometries.size(),
             merged.vertices.size(), merged.indices.size());
    }

    CacheEntry entry;
    entry.asset = mesh;
    entry.sizeBytes = sizeof(Mesh) + (mesh->getIndexCount() * sizeof(unsigned int));
    entry.lastAccessTime = 0.0f;
    meshCache[nifPath] = entry;
    currentCacheSize += entry.sizeBytes;

    return mesh;
}

std::shared_ptr<Mesh> AssetManager::loadNifFromData(const std::string& path,
                                                     const uint8_t* data, size_t dataSize) {
    // Generate a unique temp file path for the NIF data
    // Hash the path to avoid collisions
    std::string tempFile = "/data/data/com.example.oblivion/cache/bsa_nif_" +
                           std::to_string(std::hash<std::string>{}(path)) + ".nif";

    std::ofstream outFile(tempFile, std::ios::binary);
    if (!outFile.is_open()) {
        LOGE("Failed to create temp file for NIF: %s", path.c_str());
        return nullptr;
    }
    outFile.write(reinterpret_cast<const char*>(data), dataSize);
    outFile.close();

    // Parse directly from temp file (avoid recursion through loadNifMesh)
    if (!nifParser->parseFile(tempFile)) {
        LOGE("Failed to parse NIF from temp file: %s", path.c_str());
        std::remove(tempFile.c_str());
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();
    const auto geometries = nifParser->extractAllGeometry();
    const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);
    if (!merged.vertices.empty() && !merged.indices.empty()) {
        mesh->setVertices(merged.vertices);
        mesh->setIndices(merged.indices);
        mesh->uploadToGPU();

        LOGD("Mesh merged from BSA: %zu of %zu geometries, %zu vertices, %zu indices",
             merged.geometryCount, geometries.size(),
             merged.vertices.size(), merged.indices.size());
    }

    // Clean up temp file
    std::remove(tempFile.c_str());
    return mesh;
}

std::shared_ptr<Material> AssetManager::loadDDSTexture(const std::string& ddsPath) {
    LOGD("Loading DDS texture: %s", ddsPath.c_str());

    if (textureCache.find(ddsPath) != textureCache.end()) {
        LOGD("Texture found in cache: %s", ddsPath.c_str());
        textureCache[ddsPath].lastAccessTime = 0.0f;
        return std::static_pointer_cast<Material>(textureCache[ddsPath].asset);
    }

    // Try loading from BSA archives
    std::string loadPath = ddsPath;
    bool usedTempFile = false;
    std::vector<uint8_t> fileData = loadFileData(ddsPath);
    if (!fileData.empty()) {
        std::string tempFile = "/data/data/com.example.oblivion/cache/bsa_dds_" +
                               std::to_string(std::hash<std::string>{}(ddsPath)) + ".dds";
        std::ofstream outFile(tempFile, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
            outFile.close();
            loadPath = tempFile;
            usedTempFile = true;
        }
    }

    if (!ddsLoader->loadFile(loadPath)) {
        LOGE("Failed to load DDS file: %s", ddsPath.c_str());
        if (usedTempFile) std::remove(loadPath.c_str());
        return nullptr;
    }

    // Prefer a direct compressed upload: it keeps the GPU/CPU footprint at the
    // S3TC payload size instead of an RGBA8 blow-up. Falls back to the
    // decompress path when the format or the driver does not support S3TC.
    unsigned int texId = ddsLoader->uploadCompressedToGPU();
    const bool compressed = (texId != 0);
    if (!compressed) {
        if (!ddsLoader->decompressTexture()) {
            LOGE("Failed to decompress DDS texture: %s", ddsPath.c_str());
            if (usedTempFile) std::remove(loadPath.c_str());
            return nullptr;
        }
        texId = ddsLoader->uploadToGPU();
    }

    auto material = std::make_shared<Material>();
    if (texId != 0) {
        material->setTexture(texId);
        LOGD("Texture uploaded: ID=%u", texId);
    }

    // Clean up temp file
    if (usedTempFile) {
        std::remove(loadPath.c_str());
    }

    CacheEntry entry;
    entry.asset = material;
    const auto& ddsTexture = ddsLoader->getTexture();
    entry.sizeBytes = compressed
        ? ddsTexture.compressedData.size()
        : static_cast<size_t>(ddsTexture.width) * ddsTexture.height * 4;
    entry.lastAccessTime = 0.0f;
    textureCache[ddsPath] = entry;
    currentCacheSize += entry.sizeBytes;

    return material;
}

bool AssetManager::cacheExists(const std::string& key) const {
    return meshCache.find(key) != meshCache.end() || 
           textureCache.find(key) != textureCache.end();
}

void AssetManager::cacheEvict(const std::string& key) {
    auto meshIt = meshCache.find(key);
    if (meshIt != meshCache.end()) {
        currentCacheSize -= meshIt->second.sizeBytes;
        meshCache.erase(meshIt);
        LOGD("Evicted mesh from cache: %s", key.c_str());
    }

    auto texIt = textureCache.find(key);
    if (texIt != textureCache.end()) {
        currentCacheSize -= texIt->second.sizeBytes;
        textureCache.erase(texIt);
        LOGD("Evicted texture from cache: %s", key.c_str());
    }
}

size_t AssetManager::getCacheSize() const {
    return currentCacheSize;
}

void AssetManager::setCacheLimit(size_t bytes) {
    maxCacheSize = bytes;
    LOGD("Cache limit set to %zu MB", bytes / (1024 * 1024));
}

void AssetManager::pruneCache() {
    while (currentCacheSize > maxCacheSize) {
        evictLRU();
    }
}

void AssetManager::evictLRU() {
    std::string lruKey;
    float maxTime = -1.0f;

    for (auto& entry : meshCache) {
        if (entry.second.lastAccessTime > maxTime) {
            maxTime = entry.second.lastAccessTime;
            lruKey = entry.first;
        }
    }

    for (auto& entry : textureCache) {
        if (entry.second.lastAccessTime > maxTime) {
            maxTime = entry.second.lastAccessTime;
            lruKey = entry.first;
        }
    }

    if (!lruKey.empty()) {
        cacheEvict(lruKey);
    }
}

void AssetManager::clearCache() {
    meshCache.clear();
    textureCache.clear();
    missingMeshPaths.clear();
    currentCacheSize = 0;
}

bool AssetManager::isMeshKnownMissing(const std::string& nifPath) const {
    return missingMeshPaths.find(nifPath) != missingMeshPaths.end();
}

void AssetManager::markMeshMissing(const std::string& nifPath) {
    if (missingMeshPaths.size() >= MAX_MISSING_MESH_ENTRIES) return;
    missingMeshPaths.insert(nifPath);
}

void AssetManager::clearMissingMeshCache() {
    missingMeshPaths.clear();
}

std::string AssetManager::getLoadedTextureList() const {
    std::string result = "Textures: " + std::to_string(textureCache.size()) + "\n";
    for (const auto& pair : textureCache) {
        result += "  " + pair.first + " (" + std::to_string(pair.second.sizeBytes) + " bytes)\n";
    }
    return result;
}

std::string AssetManager::getLoadedModelList() const {
    std::string result = "Models: " + std::to_string(meshCache.size()) + "\n";
    for (const auto& pair : meshCache) {
        result += "  " + pair.first + " (" + std::to_string(pair.second.sizeBytes) + " bytes)\n";
    }
    return result;
}

std::string AssetManager::getTextureCacheStats() const {
    size_t totalSize = 0;
    for (const auto& pair : textureCache) {
        totalSize += pair.second.sizeBytes;
    }
    return "Texture Cache: " + std::to_string(textureCache.size()) + " items, " +
           std::to_string(totalSize) + " bytes";
}

std::string AssetManager::getModelCacheStats() const {
    size_t totalSize = 0;
    for (const auto& pair : meshCache) {
        totalSize += pair.second.sizeBytes;
    }
    return "Model Cache: " + std::to_string(meshCache.size()) + " items, " +
           std::to_string(totalSize) + " bytes";
}

std::string AssetManager::getCacheStats() const {
    return "Cache: " + std::to_string(currentCacheSize) + "/" + std::to_string(maxCacheSize) + " bytes\n" +
           "  Textures: " + std::to_string(textureCache.size()) + "\n" +
           "  Models: " + std::to_string(meshCache.size());
}

std::string AssetManager::getMemoryUsage() const {
    return "Memory: " + std::to_string(currentCacheSize) + " bytes cached\n" +
           "  Limit: " + std::to_string(maxCacheSize) + " bytes";
}

std::string AssetManager::getAssetStats() const {
    return std::string("Assets:\n") +
           "  Archives: " + std::to_string(m_archives.size()) + "\n" +
           "  Textures: " + std::to_string(textureCache.size()) + "\n" +
           "  Models: " + std::to_string(meshCache.size());
}

void AssetManager::reloadAllAssets() {
    // In a real implementation, this would reload all cached assets
    // For now, just clear the cache
    clearCache();
}
