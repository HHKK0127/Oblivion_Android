#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <cstring>
#include <android/log.h>
#include <functional>
#include <unordered_map>
#include <chrono>

#define LOG_TAG_MEMPOOL "MemoryPool"
#define LOGD_MP(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_MEMPOOL, __VA_ARGS__)
#define LOGI_MP(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MEMPOOL, __VA_ARGS__)
#define LOGW_MP(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_MEMPOOL, __VA_ARGS__)
#define LOGE_MP(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_MEMPOOL, __VA_ARGS__)

// ============================================================================
// Memory Pool - Object pool pattern for performance optimization
// ============================================================================

// Generic typed object pool
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t maxCapacity)
        : maxCapacity_(maxCapacity)
        , totalAllocated_(0)
        , activeCount_(0)
        , peakActive_(0)
    {
        preAllocate();
    }

    ~ObjectPool() {
        // All objects return to pool on destruction
        LOGI_MP("ObjectPool destroyed: peak=%zu, total=%zu", peakActive_, totalAllocated_);
    }

    // Acquire an object from the pool
    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (available_.empty()) {
            if (totalAllocated_ < maxCapacity_) {
                // Expand pool
                auto obj = std::make_unique<T>();
                T* ptr = obj.get();
                allObjects_.push_back(std::move(obj));
                totalAllocated_++;
                activeCount_++;
                if (activeCount_ > peakActive_) {
                    peakActive_ = activeCount_;
                }
                return ptr;
            }
            LOGW_MP("ObjectPool: pool exhausted (max=%zu)", maxCapacity_);
            return nullptr;
        }

        T* obj = available_.front();
        available_.pop();
        activeCount_++;
        if (activeCount_ > peakActive_) {
            peakActive_ = activeCount_;
        }
        return obj;
    }

    // Release an object back to the pool
    void release(T* obj) {
        if (!obj) return;
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push(obj);
        if (activeCount_ > 0) {
            activeCount_--;
        }
    }

    // Reset all objects (return all to pool)
    void resetAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!available_.empty()) {
            available_.pop();
        }
        for (auto& obj : allObjects_) {
            available_.push(obj.get());
        }
        activeCount_ = 0;
    }

    size_t getActiveCount() const { return activeCount_; }
    size_t getAvailableCount() const { return available_.size(); }
    size_t getTotalAllocated() const { return totalAllocated_; }
    size_t getPeakActive() const { return peakActive_; }
    size_t getMaxCapacity() const { return maxCapacity_; }

private:
    void preAllocate() {
        // Pre-allocate half of max capacity
        size_t preAllocCount = maxCapacity_ / 2;
        allObjects_.reserve(maxCapacity_);
        for (size_t i = 0; i < preAllocCount; ++i) {
            auto obj = std::make_unique<T>();
            available_.push(obj.get());
            allObjects_.push_back(std::move(obj));
        }
        totalAllocated_ = preAllocCount;
        LOGI_MP("ObjectPool pre-allocated %zu objects (max=%zu)", preAllocCount, maxCapacity_);
    }

    std::vector<std::unique_ptr<T>> allObjects_;
    std::queue<T*> available_;
    size_t maxCapacity_;
    size_t totalAllocated_;
    size_t activeCount_;
    size_t peakActive_;
    std::mutex mutex_;
};

// ============================================================================
// LRU Texture Cache Pool
// ============================================================================

class TextureCachePool {
public:
    struct CacheEntry {
        uint32_t textureId = 0;
        size_t sizeBytes = 0;
        // Use steady_clock for accurate time tracking (prevents precision loss in long sessions)
    std::chrono::steady_clock::time_point lastAccessTime = std::chrono::steady_clock::now();
        uint32_t accessCount = 0;
    };

    explicit TextureCachePool(size_t maxEntries, size_t maxMemoryBytes);
    ~TextureCachePool();

    bool initialize();
    void cleanup();

    // Cache operations
    bool contains(const std::string& key) const;
    uint32_t get(const std::string& key);
    void put(const std::string& key, uint32_t textureId, size_t sizeBytes);
    void remove(const std::string& key);
    void clear();

    // Eviction
    void evictLRU(size_t targetFreeBytes);

    // Statistics
    size_t getEntryCount() const { return entries_.size(); }
    size_t getMemoryUsed() const { return currentMemoryBytes_; }
    size_t getMemoryMax() const { return maxMemoryBytes_; }
    float getHitRate() const;
    size_t getHitCount() const { return hitCount_; }
    size_t getMissCount() const { return missCount_; }

private:
    std::unordered_map<std::string, CacheEntry> entries_;
    size_t maxEntries_;
    size_t maxMemoryBytes_;
    size_t currentMemoryBytes_;
    size_t hitCount_;
    size_t missCount_;
    mutable std::mutex mutex_;

    std::string findLRUKey() const;
    std::chrono::steady_clock::time_point getCurrentTime() const;
};

// ============================================================================
// Memory Pool Manager - Central pool management
// ============================================================================

class MemoryPoolManager {
public:
    // Pool sizes
    static constexpr size_t NPC_POOL_SIZE = 100;
    static constexpr size_t EFFECT_POOL_SIZE = 50;
    static constexpr size_t TEXTURE_CACHE_ENTRIES = 256;
    static constexpr size_t TEXTURE_CACHE_MEMORY = 64 * 1024 * 1024; // 64MB

    MemoryPoolManager();
    ~MemoryPoolManager();

    bool initialize();
    void cleanup();

    // NPC pool (placeholder struct for pooled NPC data)
    struct PooledNPC {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float rotation = 0.0f;
        int32_t health = 100;
        int32_t entityId = -1;
        bool active = false;
        void reset() {
            position[0] = position[1] = position[2] = 0.0f;
            rotation = 0.0f;
            health = 100;
            entityId = -1;
            active = false;
        }
    };

    // Effect pool (placeholder struct for pooled effect data)
    struct PooledEffect {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float lifetime = 0.0f;
        float maxLifetime = 1.0f;
        int32_t effectType = 0;
        bool active = false;
        void reset() {
            position[0] = position[1] = position[2] = 0.0f;
            lifetime = 0.0f;
            maxLifetime = 1.0f;
            effectType = 0;
            active = false;
        }
    };

    // Pool accessors
    ObjectPool<PooledNPC>& getNPCPool() { return *npcPool_; }
    ObjectPool<PooledEffect>& getEffectPool() { return *effectPool_; }
    TextureCachePool& getTextureCache() { return *textureCache_; }

    // Statistics
    struct PoolStats {
        size_t npcActive;
        size_t npcPeak;
        size_t effectActive;
        size_t effectPeak;
        size_t textureEntries;
        size_t textureMemoryBytes;
        float textureHitRate;
    };

    PoolStats getStats() const;

private:
    std::unique_ptr<ObjectPool<PooledNPC>> npcPool_;
    std::unique_ptr<ObjectPool<PooledEffect>> effectPool_;
    std::unique_ptr<TextureCachePool> textureCache_;
    bool initialized_ = false;
};
