#include "cache_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

// ============================================================================
// CacheManager Implementation
// ============================================================================

CacheManager::CacheManager() = default;

CacheManager::~CacheManager() {
    cleanup();
}

bool CacheManager::initialize(const Config& config) {
    if (initialized_) return true;

    config_ = config;

    LOGI_CM("Initializing CacheManager:");
    LOGI_CM("  L1: %zuMB, max %zu entries",
            config_.l1MaxMemoryBytes / (1024 * 1024), config_.l1MaxEntries);
    LOGI_CM("  L2: %zuMB, max %zu entries",
            config_.l2MaxDiskBytes / (1024 * 1024), config_.l2MaxEntries);
    LOGI_CM("  Disk path: %s", config_.diskCachePath.c_str());

    // Create disk cache directory if needed
    if (!config_.diskCachePath.empty()) {
        mkdir(config_.diskCachePath.c_str(), 0755);
    }

    initialized_ = true;
    return true;
}

void CacheManager::cleanup() {
    if (!initialized_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    l1Cache_.clear();
    l1LRUList_.clear();
    l1MemoryBytes_ = 0;

    l2Cache_.clear();
    l2DiskBytes_ = 0;

    initialized_ = false;
    LOGI_CM("CacheManager cleaned up");
}

// ============================================================================
// Cache Operations
// ============================================================================

bool CacheManager::get(const std::string& key, std::vector<uint8_t>& outData) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Try L1 first
    auto l1It = l1Cache_.find(key);
    if (l1It != l1Cache_.end()) {
        touchL1(key);
        l1It->second.first.accessCount++;
        l1It->second.first.lastAccessTime = getTime();
        outData = l1It->second.first.data;
        l1Hits_++;
        return true;
    }
    l1Misses_++;

    // Try L2
    auto l2It = l2Cache_.find(key);
    if (l2It != l2Cache_.end()) {
        l2It->second.accessCount++;
        l2It->second.lastAccessTime = getTime();
        outData = l2It->second.data;
        l2Hits_++;

        // Promote to L1 if frequently accessed
        if (l2It->second.accessCount >= 3) {
            promoteToL1(key, l2It->second);
        }
        return true;
    }
    l2Misses_++;

    // Try disk L2
    CacheEntry diskEntry;
    if (readFromDisk(key, diskEntry)) {
        diskEntry.accessCount = 1;
        diskEntry.lastAccessTime = getTime();
        outData = diskEntry.data;
        l2Cache_[key] = diskEntry;
        l2DiskBytes_ += diskEntry.sizeBytes;
        l2Hits_++;
        return true;
    }

    return false;
}

void CacheManager::put(const std::string& key, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    CacheEntry entry;
    entry.data = data;
    entry.sizeBytes = data.size();
    entry.lastAccessTime = getTime();
    entry.creationTime = getTime();
    entry.accessCount = 1;

    // Add to L1
    auto it = l1Cache_.find(key);
    if (it != l1Cache_.end()) {
        // Update existing entry
        l1MemoryBytes_ -= it->second.first.sizeBytes;
        l1LRUList_.erase(it->second.second);
        l1Cache_.erase(it);
    }

    // Ensure L1 has space
    while (l1MemoryBytes_ + entry.sizeBytes > config_.l1MaxMemoryBytes ||
           l1Cache_.size() >= config_.l1MaxEntries) {
        if (l1LRUList_.empty()) break;
        // Evict LRU
        const std::string& evictKey = l1LRUList_.back();
        auto evictIt = l1Cache_.find(evictKey);
        if (evictIt != l1Cache_.end()) {
            demoteToL2(evictKey, evictIt->second.first);
            l1MemoryBytes_ -= evictIt->second.first.sizeBytes;
            l1Cache_.erase(evictIt);
            l1LRUList_.pop_back();
        }
    }

    // Insert into L1
    l1LRUList_.push_front(key);
    l1Cache_[key] = {entry, l1LRUList_.begin()};
    l1MemoryBytes_ += entry.sizeBytes;
}

void CacheManager::put(const std::string& key, std::vector<uint8_t>&& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    CacheEntry entry;
    entry.sizeBytes = data.size();
    entry.data = std::move(data);
    entry.lastAccessTime = getTime();
    entry.creationTime = getTime();
    entry.accessCount = 1;

    // Same logic as above but with move semantics
    auto it = l1Cache_.find(key);
    if (it != l1Cache_.end()) {
        l1MemoryBytes_ -= it->second.first.sizeBytes;
        l1LRUList_.erase(it->second.second);
        l1Cache_.erase(it);
    }

    while (l1MemoryBytes_ + entry.sizeBytes > config_.l1MaxMemoryBytes ||
           l1Cache_.size() >= config_.l1MaxEntries) {
        if (l1LRUList_.empty()) break;
        const std::string& evictKey = l1LRUList_.back();
        auto evictIt = l1Cache_.find(evictKey);
        if (evictIt != l1Cache_.end()) {
            demoteToL2(evictKey, evictIt->second.first);
            l1MemoryBytes_ -= evictIt->second.first.sizeBytes;
            l1Cache_.erase(evictIt);
            l1LRUList_.pop_back();
        }
    }

    l1LRUList_.push_front(key);
    l1Cache_[key] = {std::move(entry), l1LRUList_.begin()};
    l1MemoryBytes_ += l1Cache_[key].first.sizeBytes;
}

bool CacheManager::contains(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return l1Cache_.find(key) != l1Cache_.end() ||
           l2Cache_.find(key) != l2Cache_.end();
}

void CacheManager::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto l1It = l1Cache_.find(key);
    if (l1It != l1Cache_.end()) {
        l1MemoryBytes_ -= l1It->second.first.sizeBytes;
        l1LRUList_.erase(l1It->second.second);
        l1Cache_.erase(l1It);
    }

    auto l2It = l2Cache_.find(key);
    if (l2It != l2Cache_.end()) {
        l2DiskBytes_ -= l2It->second.sizeBytes;
        l2Cache_.erase(l2It);
    }

    deleteFromDisk(key);
}

void CacheManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    l1Cache_.clear();
    l1LRUList_.clear();
    l1MemoryBytes_ = 0;
    l2Cache_.clear();
    l2DiskBytes_ = 0;
    LOGI_CM("All caches cleared");
}

// ============================================================================
// Maintenance
// ============================================================================

void CacheManager::update(float deltaTime) {
    currentTime_ += deltaTime;
    timeSinceCleanup_ += deltaTime;

    if (timeSinceCleanup_ >= config_.cleanupIntervalSec) {
        cleanupExpired();
        timeSinceCleanup_ = 0.0f;
    }
}

void CacheManager::cleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    float now = getTime();
    size_t removed = 0;

    // Cleanup L2 expired entries
    auto it = l2Cache_.begin();
    while (it != l2Cache_.end()) {
        if (now - it->second.lastAccessTime > config_.entryMaxAgeSec) {
            l2DiskBytes_ -= it->second.sizeBytes;
            deleteFromDisk(it->first);
            it = l2Cache_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }

    if (removed > 0) {
        LOGI_CM("Cleanup: removed %zu expired L2 entries", removed);
    }
}

void CacheManager::evictL1(size_t targetBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t freed = 0;

    while (freed < targetBytes && !l1LRUList_.empty()) {
        const std::string& evictKey = l1LRUList_.back();
        auto it = l1Cache_.find(evictKey);
        if (it != l1Cache_.end()) {
            demoteToL2(evictKey, it->second.first);
            freed += it->second.first.sizeBytes;
            l1MemoryBytes_ -= it->second.first.sizeBytes;
            l1Cache_.erase(it);
        }
        l1LRUList_.pop_back();
    }

    LOGI_CM("L1 eviction: freed %zu bytes", freed);
}

// ============================================================================
// Statistics
// ============================================================================

CacheManager::CacheStats CacheManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CacheStats stats;
    stats.l1EntryCount = l1Cache_.size();
    stats.l1MemoryBytes = l1MemoryBytes_;
    stats.l1MaxMemoryBytes = config_.l1MaxMemoryBytes;
    stats.l1Hits = l1Hits_;
    stats.l1Misses = l1Misses_;

    size_t l1Total = l1Hits_ + l1Misses_;
    stats.l1HitRate = l1Total > 0 ?
        static_cast<float>(l1Hits_) / static_cast<float>(l1Total) * 100.0f : 0.0f;

    stats.l2EntryCount = l2Cache_.size();
    stats.l2DiskBytes = l2DiskBytes_;
    stats.l2MaxDiskBytes = config_.l2MaxDiskBytes;
    stats.l2Hits = l2Hits_;
    stats.l2Misses = l2Misses_;

    size_t l2Total = l2Hits_ + l2Misses_;
    stats.l2HitRate = l2Total > 0 ?
        static_cast<float>(l2Hits_) / static_cast<float>(l2Total) * 100.0f : 0.0f;

    stats.totalHits = l1Hits_ + l2Hits_;
    stats.totalMisses = l1Misses_ + l2Misses_;
    size_t total = stats.totalHits + stats.totalMisses;
    stats.overallHitRate = total > 0 ?
        static_cast<float>(stats.totalHits) / static_cast<float>(total) * 100.0f : 0.0f;

    return stats;
}

void CacheManager::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    l1Hits_ = 0;
    l1Misses_ = 0;
    l2Hits_ = 0;
    l2Misses_ = 0;
}

// ============================================================================
// Private Methods
// ============================================================================

void CacheManager::promoteToL1(const std::string& key, const CacheEntry& entry) {
    // Ensure L1 has space
    while (l1MemoryBytes_ + entry.sizeBytes > config_.l1MaxMemoryBytes ||
           l1Cache_.size() >= config_.l1MaxEntries) {
        if (l1LRUList_.empty()) break;
        const std::string& evictKey = l1LRUList_.back();
        auto evictIt = l1Cache_.find(evictKey);
        if (evictIt != l1Cache_.end()) {
            demoteToL2(evictKey, evictIt->second.first);
            l1MemoryBytes_ -= evictIt->second.first.sizeBytes;
            l1Cache_.erase(evictIt);
            l1LRUList_.pop_back();
        }
    }

    l1LRUList_.push_front(key);
    CacheEntry copy = entry;
    l1Cache_[key] = {std::move(copy), l1LRUList_.begin()};
    l1MemoryBytes_ += entry.sizeBytes;
}

void CacheManager::demoteToL2(const std::string& key, const CacheEntry& entry) {
    // Only keep in L2 memory if there's space
    if (l2DiskBytes_ + entry.sizeBytes <= config_.l2MaxDiskBytes &&
        l2Cache_.size() < config_.l2MaxEntries) {
        l2Cache_[key] = entry;
        l2DiskBytes_ += entry.sizeBytes;
    }

    // Write to disk
    writeToDisk(key, entry);
}

bool CacheManager::writeToDisk(const std::string& key, const CacheEntry& entry) {
    if (config_.diskCachePath.empty()) return false;

    std::string path = getDiskPath(key);
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // Write header: size
    uint32_t size = static_cast<uint32_t>(entry.sizeBytes);
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.write(reinterpret_cast<const char*>(entry.data.data()), entry.sizeBytes);
    file.flush();  // Ensure data is written to disk before returning
    return file.good();
}

bool CacheManager::readFromDisk(const std::string& key, CacheEntry& entry) {
    if (config_.diskCachePath.empty()) return false;

    std::string path = getDiskPath(key);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint32_t size = 0;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (size == 0 || size > 10 * 1024 * 1024) return false; // Max 10MB per entry

    entry.data.resize(size);
    file.read(reinterpret_cast<char*>(entry.data.data()), size);
    entry.sizeBytes = size;
    return file.good();
}

bool CacheManager::deleteFromDisk(const std::string& key) {
    if (config_.diskCachePath.empty()) return false;
    std::string path = getDiskPath(key);
    return ::remove(path.c_str()) == 0;
}

std::string CacheManager::getDiskPath(const std::string& key) const {
    return config_.diskCachePath + "/" + hashKey(key) + ".cache";
}

std::string CacheManager::hashKey(const std::string& key) const {
    // Simple hash for filename
    size_t hash = std::hash<std::string>{}(key);
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

void CacheManager::touchL1(const std::string& key) {
    auto it = l1Cache_.find(key);
    if (it != l1Cache_.end()) {
        l1LRUList_.erase(it->second.second);
        l1LRUList_.push_front(key);
        it->second.second = l1LRUList_.begin();
    }
}

float CacheManager::getTime() const {
    using namespace std::chrono;
    return duration<float>(steady_clock::now().time_since_epoch()).count();
}
