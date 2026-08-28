#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cassert>
#include <cstring>
#include <android/log.h>

#define LOG_TAG_MD "MemoryDefrag"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_MD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_MD, __VA_ARGS__)
#else
#define LOGD_MD(...) do {} while(0)
#endif
#define LOGI_MD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MD, __VA_ARGS__)
#define LOGW_MD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_MD, __VA_ARGS__)
#define LOGE_MD(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_MD, __VA_ARGS__)

// ============================================================================
// Memory Defragmentation System
// Phase 55: Pool allocator with compaction and handle-based indirection
// Android page-aligned (4KB)
// ============================================================================

namespace engine {

static constexpr size_t PAGE_SIZE = 4096;
static constexpr size_t MAX_HANDLES = 4096;

// Generation counter for use-after-free detection
struct HandleGen {
    uint32_t index = 0;
    uint32_t generation = 0;

    bool isValid() const { return index != 0xFFFFFFFF; }
    bool operator==(const HandleGen& o) const {
        return index == o.index && generation == o.generation;
    }
};

// Typed handle wrapper
template<typename T>
struct Handle {
    HandleGen gen;

    bool isValid() const { return gen.isValid(); }
    uint32_t getIndex() const { return gen.index; }
    uint32_t getGeneration() const { return gen.generation; }
};

// Free block in the pool
struct FreeBlock {
    size_t offset;
    size_t size;
};

// Allocation metadata
struct AllocRecord {
    size_t offset;
    size_t size;
    uint32_t generation;
    bool alive;
};

// ============================================================================
// MemoryDefrag - pool allocator with compaction support
// ============================================================================

class MemoryDefrag {
public:
    explicit MemoryDefrag(size_t poolSize = 16 * 1024 * 1024) // 16MB default
        : poolSize_(alignToPage(poolSize)) {
        // Allocate page-aligned pool
        pool_ = static_cast<uint8_t*>(
            ::aligned_alloc(PAGE_SIZE, poolSize_));
        if (pool_) {
            std::memset(pool_, 0, poolSize_);
            freeList_.push_back({0, poolSize_});
            LOGI_MD("Initialized pool: %zu bytes (%zu pages)",
                    poolSize_, poolSize_ / PAGE_SIZE);
        } else {
            LOGE_MD("Failed to allocate pool of %zu bytes", poolSize_);
        }

        // Initialize handle table
        handleTable_.resize(MAX_HANDLES);
        for (uint32_t i = 0; i < MAX_HANDLES; i++) {
            freeHandleIndices_.push_back(i);
        }
    }

    ~MemoryDefrag() {
        if (pool_) {
            ::free(pool_);
            pool_ = nullptr;
        }
    }

    // Non-copyable
    MemoryDefrag(const MemoryDefrag&) = delete;
    MemoryDefrag& operator=(const MemoryDefrag&) = delete;

    // --- Allocation ---

    template<typename T>
    Handle<T> allocate(size_t count = 1) {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t requestedSize = sizeof(T) * count;
        size_t alignedSize = alignToPage(requestedSize);

        // Find best-fit free block
        int bestIdx = -1;
        size_t bestSize = SIZE_MAX;
        for (int i = 0; i < static_cast<int>(freeList_.size()); i++) {
            if (freeList_[i].size >= alignedSize && freeList_[i].size < bestSize) {
                bestIdx = i;
                bestSize = freeList_[i].size;
            }
        }

        if (bestIdx < 0) {
            LOGW_MD("Allocation failed: %zu bytes requested, no suitable block", requestedSize);
            Handle<T> h;
            h.gen.index = 0xFFFFFFFF;
            h.gen.generation = 0;
            return h;
        }

        // Allocate from free block
        size_t offset = freeList_[bestIdx].offset;
        size_t remainder = freeList_[bestIdx].size - alignedSize;

        if (remainder > 0) {
            freeList_[bestIdx] = {offset + alignedSize, remainder};
        } else {
            freeList_.erase(freeList_.begin() + bestIdx);
        }

        // Get handle
        uint32_t handleIdx = allocHandle();
        if (handleIdx == 0xFFFFFFFF) {
            // Rollback: return block to free list
            mergeIntoFreeList({offset, alignedSize});
            Handle<T> h;
            h.gen.index = 0xFFFFFFFF;
            h.gen.generation = 0;
            return h;
        }

        AllocRecord& rec = handleTable_[handleIdx];
        rec.offset = offset;
        rec.size = alignedSize;
        rec.generation++;
        rec.alive = true;

        Handle<T> h;
        h.gen.index = handleIdx;
        h.gen.generation = rec.generation;

        totalAllocated_ += alignedSize;
        allocCount_++;

        return h;
    }

    template<typename T>
    void deallocate(Handle<T> handle) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!validateHandle(handle)) return;

        AllocRecord& rec = handleTable_[handle.gen.index];
        rec.alive = false;

        mergeIntoFreeList({rec.offset, rec.size});
        totalAllocated_ -= rec.size;
        allocCount_--;

        freeHandle(handle.gen.index);
    }

    // --- Pointer access (through handle indirection) ---

    template<typename T>
    T* resolve(Handle<T> handle) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!validateHandle(handle)) return nullptr;

        AllocRecord& rec = handleTable_[handle.gen.index];
        if (!rec.alive) return nullptr;

        return reinterpret_cast<T*>(pool_ + rec.offset);
    }

    // --- Defragmentation ---

    // Compact live allocations, moving them toward the start of the pool
    // Returns number of blocks moved
    uint32_t defragment() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!pool_) return 0;

        uint32_t movedCount = 0;
        size_t writeOffset = 0;

        // Collect all live allocations sorted by offset
        std::vector<uint32_t> liveHandles;
        for (uint32_t i = 0; i < MAX_HANDLES; i++) {
            if (handleTable_[i].alive) {
                liveHandles.push_back(i);
            }
        }

        // Sort by offset for sequential compaction
        std::sort(liveHandles.begin(), liveHandles.end(),
            [this](uint32_t a, uint32_t b) {
                return handleTable_[a].offset < handleTable_[b].offset;
            });

        // Move each live block to the write position
        for (uint32_t idx : liveHandles) {
            AllocRecord& rec = handleTable_[idx];

            if (rec.offset != writeOffset) {
                // Move data
                std::memmove(pool_ + writeOffset, pool_ + rec.offset, rec.size);
                rec.offset = writeOffset;
                movedCount++;
            }

            writeOffset += rec.size;
        }

        // Rebuild free list with single remaining block
        freeList_.clear();
        if (writeOffset < poolSize_) {
            freeList_.push_back({writeOffset, poolSize_ - writeOffset});
        }

        LOGI_MD("Defragmentation complete: %u blocks moved, %zu bytes coalesced",
                movedCount, poolSize_ - writeOffset);

        return movedCount;
    }

    // --- Statistics ---

    struct Stats {
        size_t poolSize;
        size_t totalAllocated;
        size_t totalFree;
        size_t largestFreeBlock;
        uint32_t allocCount;
        uint32_t freeBlockCount;
        float fragmentationRatio;  // 0.0 = no fragmentation, 1.0 = fully fragmented
    };

    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        Stats s{};
        s.poolSize = poolSize_;
        s.totalAllocated = totalAllocated_;
        s.totalFree = poolSize_ - totalAllocated_;
        s.allocCount = allocCount_;
        s.freeBlockCount = static_cast<uint32_t>(freeList_.size());

        s.largestFreeBlock = 0;
        for (const auto& block : freeList_) {
            if (block.size > s.largestFreeBlock) {
                s.largestFreeBlock = block.size;
            }
        }

        // Fragmentation: 1 - (largest_free / total_free)
        if (s.totalFree > 0) {
            s.fragmentationRatio = 1.0f -
                static_cast<float>(s.largestFreeBlock) / static_cast<float>(s.totalFree);
        } else {
            s.fragmentationRatio = 0.0f;
        }

        return s;
    }

    size_t getPoolSize() const { return poolSize_; }
    size_t getAllocatedBytes() const { return totalAllocated_; }
    uint32_t getAllocationCount() const { return allocCount_; }

private:
    uint8_t* pool_ = nullptr;
    size_t poolSize_;
    size_t totalAllocated_ = 0;
    uint32_t allocCount_ = 0;

    std::vector<FreeBlock> freeList_;
    std::vector<AllocRecord> handleTable_;
    std::vector<uint32_t> freeHandleIndices_;
    mutable std::mutex mutex_;

    static size_t alignToPage(size_t size) {
        return (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    uint32_t allocHandle() {
        if (freeHandleIndices_.empty()) {
            LOGE_MD("Handle table exhausted!");
            return 0xFFFFFFFF;
        }
        uint32_t idx = freeHandleIndices_.back();
        freeHandleIndices_.pop_back();
        return idx;
    }

    void freeHandle(uint32_t idx) {
        freeHandleIndices_.push_back(idx);
    }

    template<typename T>
    bool validateHandle(Handle<T> handle) const {
        if (handle.gen.index >= MAX_HANDLES) return false;
        const AllocRecord& rec = handleTable_[handle.gen.index];
        return rec.alive && rec.generation == handle.gen.generation;
    }

    void mergeIntoFreeList(FreeBlock block) {
        // Insert and coalesce adjacent blocks
        auto it = freeList_.begin();
        while (it != freeList_.end()) {
            if (it->offset + it->size == block.offset) {
                // Merge with previous
                block.offset = it->offset;
                block.size += it->size;
                it = freeList_.erase(it);
            } else if (block.offset + block.size == it->offset) {
                // Merge with next
                block.size += it->size;
                it = freeList_.erase(it);
            } else {
                ++it;
            }
        }
        freeList_.push_back(block);
    }
};

} // namespace engine
