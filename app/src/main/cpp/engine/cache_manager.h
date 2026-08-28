#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>
#include <cstdint>
#include <chrono>
#include <functional>
#include <android/log.h>

#define LOG_TAG_CACHEMGR "CacheManager"
#define LOGD_CM(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_CACHEMGR, __VA_ARGS__)
#define LOGI_CM(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_CACHEMGR, __VA_ARGS__)
#define LOGW_CM(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_CACHEMGR, __VA_ARGS__)
#define LOGE_CM(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_CACHEMGR, __VA_ARGS__)

// ============================================================================
// Cache Manager - Multi-layer cache (L1 memory + L2 disk)
// ============================================================================

class CacheManager {
public:
    // ========================================================================
    // Cache entry
    // ========================================================================

    struct CacheEntry {
        std::vector<uint8_t> data;
        size_t sizeBytes = 0;
        float lastAccessTime = 0.0f;
        float creationTime = 0.0f;
        uint32_t accessCount = 0;
        bool dirty = false;
    };

    // ========================================================================
    // Cache statistics
    // ========================================================================

    struct CacheStats {
        // L1 Memory cache
        size_t l1EntryCount = 0;
        size_t l1MemoryBytes = 0;
        size_t l1MaxMemoryBytes = 0;
        size_t l1Hits = 0;
        size_t l1Misses = 0;
        float l1HitRate = 0.0f;

        // L2 Disk cache
        size_t l2EntryCount = 0;
        size_t l2DiskBytes = 0;
        size_t l2MaxDiskBytes = 0;
        size_t l2Hits = 0;
        size_t l2Misses = 0;
        float l2HitRate = 0.0f;

        // Overall
        size_t totalHits = 0;
        size_t totalMisses = 0;
        float overallHitRate = 0.0f;
    };

    // ========================================================================
    // Configuration
    // ========================================================================

    struct Config {
        size_t l1MaxMemoryBytes = 32 * 1024 * 1024;   // 32MB L1
        size_t l2MaxDiskBytes = 128 * 1024 * 1024;     // 128MB L2
        size_t l1MaxEntries = 512;
        size_t l2MaxEntries = 2048;
        float cleanupIntervalSec = 30.0f;
        float entryMaxAgeSec = 300.0f;                  // 5 minutes
        std::string diskCachePath;
    };

    CacheManager();
    ~CacheManager();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool initialize(const Config& config);
    void cleanup();

    // ========================================================================
    // Cache operations
    // ========================================================================

    // Get data from cache (checks L1 first, then L2)
    bool get(const std::string& key, std::vector<uint8_t>& outData);

    // Put data into cache (writes to L1, promotes to L2 if frequently accessed)
    void put(const std::string& key, const std::vector<uint8_t>& data);

    // Put data into cache (move variant)
    void put(const std::string& key, std::vector<uint8_t>&& data);

    // Check if key exists in any cache layer
    bool contains(const std::string& key) const;

    // Remove entry from all layers
    void remove(const std::string& key);

    // Clear all caches
    void clear();

    // ========================================================================
    // Maintenance
    // ========================================================================

    // Periodic cleanup (call from game loop)
    void update(float deltaTime);

    // Force cleanup of expired entries
    void cleanupExpired();

    // Evict L1 entries to free memory
    void evictL1(size_t targetBytes);

    // ========================================================================
    // Statistics
    // ========================================================================

    CacheStats getStats() const;
    void resetStats();

private:
    // ========================================================================
    // L1 Memory Cache (LRU)
    // ========================================================================

    using LRUIterator = std::list<std::string>::iterator;

    std::unordered_map<std::string, std::pair<CacheEntry, LRUIterator>> l1Cache_;
    std::list<std::string> l1LRUList_;
    size_t l1MemoryBytes_ = 0;

    // ========================================================================
    // L2 Disk Cache
    // ========================================================================

    std::unordered_map<std::string, CacheEntry> l2Cache_;
    size_t l2DiskBytes_ = 0;

    // ========================================================================
    // Configuration and state
    // ========================================================================

    Config config_;
    bool initialized_ = false;
    float timeSinceCleanup_ = 0.0f;
    float currentTime_ = 0.0f;

    // Statistics
    size_t l1Hits_ = 0;
    size_t l1Misses_ = 0;
    size_t l2Hits_ = 0;
    size_t l2Misses_ = 0;

    mutable std::mutex mutex_;

    // ========================================================================
    // Private methods
    // ========================================================================

    // Promote L2 entry to L1
    void promoteToL1(const std::string& key, const CacheEntry& entry);

    // Demote L1 entry to L2
    void demoteToL2(const std::string& key, const CacheEntry& entry);

    // Write entry to disk (L2)
    bool writeToDisk(const std::string& key, const CacheEntry& entry);

    // Read entry from disk (L2)
    bool readFromDisk(const std::string& key, CacheEntry& entry);

    // Delete entry from disk
    bool deleteFromDisk(const std::string& key);

    // Get disk path for a key
    std::string getDiskPath(const std::string& key) const;

    // Hash key for filename
    std::string hashKey(const std::string& key) const;

    // Update L1 LRU position
    void touchL1(const std::string& key);

    // Get current time
    float getTime() const;
};
