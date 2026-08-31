#include "memory_pool.h"
#include <algorithm>

// ============================================================================
// TextureCachePool Implementation
// ============================================================================

TextureCachePool::TextureCachePool(size_t maxEntries, size_t maxMemoryBytes)
    : maxEntries_(maxEntries)
    , maxMemoryBytes_(maxMemoryBytes)
    , currentMemoryBytes_(0)
    , hitCount_(0)
    , missCount_(0)
{
}

TextureCachePool::~TextureCachePool() {
    cleanup();
}

bool TextureCachePool::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    currentMemoryBytes_ = 0;
    hitCount_ = 0;
    missCount_ = 0;
    LOGI_MP("TextureCachePool initialized: maxEntries=%zu, maxMemory=%zuMB",
            maxEntries_, maxMemoryBytes_ / (1024 * 1024));
    return true;
}

void TextureCachePool::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    currentMemoryBytes_ = 0;
    LOGI_MP("TextureCachePool cleaned up");
}

bool TextureCachePool::contains(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.find(key) != entries_.end();
}

uint32_t TextureCachePool::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.lastAccessTime = getCurrentTime();
        it->second.accessCount++;
        hitCount_++;
        return it->second.textureId;
    }
    missCount_++;
    return 0;
}

void TextureCachePool::put(const std::string& key, uint32_t textureId, size_t sizeBytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Guard: single texture larger than max cache cannot be cached
    if (sizeBytes > maxMemoryBytes_) {
        LOGE_MP("Texture too large to cache: %zu > %zu", sizeBytes, maxMemoryBytes_);
        return; // Cannot cache - would cause infinite eviction loop
    }

    // Check if we need to evict
    size_t evictionAttempts = 0;
    size_t maxEvictionAttempts = entries_.size() + 1; // Safety limit
    while ((entries_.size() >= maxEntries_ ||
            currentMemoryBytes_ + sizeBytes > maxMemoryBytes_) &&
           !entries_.empty() && evictionAttempts < maxEvictionAttempts) {
        std::string lruKey = findLRUKey();
        if (lruKey.empty()) break;
        auto it = entries_.find(lruKey);
        if (it != entries_.end()) {
            currentMemoryBytes_ -= it->second.sizeBytes;
            entries_.erase(it);
            LOGD_MP("Evicted texture: %s", lruKey.c_str());
        }
        evictionAttempts++;
    }

    // Remove existing entry if updating
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        currentMemoryBytes_ -= it->second.sizeBytes;
    }

    CacheEntry entry;
    entry.textureId = textureId;
    entry.sizeBytes = sizeBytes;
    entry.lastAccessTime = getCurrentTime();
    entry.accessCount = 1;
    entries_[key] = entry;
    currentMemoryBytes_ += sizeBytes;
}

void TextureCachePool::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        currentMemoryBytes_ -= it->second.sizeBytes;
        entries_.erase(it);
    }
}

void TextureCachePool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    currentMemoryBytes_ = 0;
}

void TextureCachePool::evictLRU(size_t targetFreeBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t freed = 0;
    while (freed < targetFreeBytes && !entries_.empty()) {
        std::string lruKey = findLRUKey();
        if (lruKey.empty()) break;
        auto it = entries_.find(lruKey);
        if (it != entries_.end()) {
            freed += it->second.sizeBytes;
            currentMemoryBytes_ -= it->second.sizeBytes;
            entries_.erase(it);
        }
    }
    LOGI_MP("LRU eviction: freed %zu bytes", freed);
}

float TextureCachePool::getHitRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = hitCount_ + missCount_;
    if (total == 0) return 0.0f;
    return static_cast<float>(hitCount_) / static_cast<float>(total) * 100.0f;
}

std::string TextureCachePool::findLRUKey() const {
    std::string lruKey;
    auto oldestTime = std::chrono::steady_clock::time_point::max();
    uint32_t lowestAccess = std::numeric_limits<uint32_t>::max();

    for (const auto& pair : entries_) {
        // Prefer least recently used, then least frequently used
        if (pair.second.lastAccessTime < oldestTime ||
            (pair.second.lastAccessTime == oldestTime &&
             pair.second.accessCount < lowestAccess)) {
            oldestTime = pair.second.lastAccessTime;
            lowestAccess = pair.second.accessCount;
            lruKey = pair.first;
        }
    }
    return lruKey;
}

std::chrono::steady_clock::time_point TextureCachePool::getCurrentTime() const {
    return std::chrono::steady_clock::now();
}

// ============================================================================
// MemoryPoolManager Implementation
// ============================================================================

MemoryPoolManager::MemoryPoolManager() = default;

MemoryPoolManager::~MemoryPoolManager() {
    cleanup();
}

bool MemoryPoolManager::initialize() {
    if (initialized_) return true;

    LOGI_MP("Initializing MemoryPoolManager...");

    // Create NPC pool
    npcPool_ = std::make_unique<ObjectPool<PooledNPC>>(NPC_POOL_SIZE);

    // Create Effect pool
    effectPool_ = std::make_unique<ObjectPool<PooledEffect>>(EFFECT_POOL_SIZE);

    // Create Texture cache pool
    textureCache_ = std::make_unique<TextureCachePool>(
        TEXTURE_CACHE_ENTRIES, TEXTURE_CACHE_MEMORY);
    if (!textureCache_->initialize()) {
        LOGE_MP("Failed to initialize TextureCachePool");
        return false;
    }

    initialized_ = true;
    LOGI_MP("MemoryPoolManager initialized successfully");
    return true;
}

void MemoryPoolManager::cleanup() {
    if (!initialized_) return;

    LOGI_MP("Cleaning up MemoryPoolManager...");

    if (textureCache_) {
        textureCache_->cleanup();
        textureCache_.reset();
    }
    if (effectPool_) {
        effectPool_.reset();
    }
    if (npcPool_) {
        npcPool_.reset();
    }

    initialized_ = false;
    LOGI_MP("MemoryPoolManager cleaned up");
}

MemoryPoolManager::PoolStats MemoryPoolManager::getStats() const {
    PoolStats stats = {};
    if (npcPool_) {
        stats.npcActive = npcPool_->getActiveCount();
        stats.npcPeak = npcPool_->getPeakActive();
    }
    if (effectPool_) {
        stats.effectActive = effectPool_->getActiveCount();
        stats.effectPeak = effectPool_->getPeakActive();
    }
    if (textureCache_) {
        stats.textureEntries = textureCache_->getEntryCount();
        stats.textureMemoryBytes = textureCache_->getMemoryUsed();
        stats.textureHitRate = textureCache_->getHitRate();
    }
    return stats;
}
