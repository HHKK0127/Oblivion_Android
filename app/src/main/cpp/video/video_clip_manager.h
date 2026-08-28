#pragma once

// ============================================================================
// VideoClipManager - manages video clip preloading and memory
// Phase 53: Bink Video replacement
// ============================================================================

#include "bink_video_player.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>

namespace oblivion {
namespace video {

// ============================================================================
// Clip loading state
// ============================================================================

enum class ClipLoadState {
    UNLOADED,
    LOADING,
    LOADED,
    FAILED
};

// ============================================================================
// Managed clip entry
// ============================================================================

struct ManagedClip {
    VideoClip clip;
    ClipLoadState loadState = ClipLoadState::UNLOADED;
    int refCount = 0;
    int64_t memoryUsageBytes = 0;
    std::chrono::steady_clock::time_point lastAccessTime;
    bool pinned = false;  // Pinned clips are never auto-unloaded
};

// ============================================================================
// VideoClipManager - clip lifecycle and memory management
// ============================================================================

class VideoClipManager {
public:
    VideoClipManager();
    ~VideoClipManager();

    // Register a clip for management (does not load into memory yet)
    bool registerClip(const VideoClip& clip);

    // Preload clip data into memory (async-capable)
    bool preloadClip(const std::string& clipId);

    // Unload clip from memory
    bool unloadClip(const std::string& clipId);

    // Unload all clips
    void unloadAll();

    // Pin a clip to prevent auto-unload
    bool pinClip(const std::string& clipId);

    // Unpin a clip
    bool unpinClip(const std::string& clipId);

    // Get clip info
    const ManagedClip* getManagedClip(const std::string& clipId) const;

    // Get all registered clip IDs
    std::vector<std::string> getRegisteredClipIds() const;

    // Get loaded clip IDs
    std::vector<std::string> getLoadedClipIds() const;

    // Memory management
    int64_t getTotalMemoryUsage() const;
    int64_t getMaxMemoryBudget() const { return maxMemoryBudgetBytes_; }
    void setMaxMemoryBudget(int64_t bytes) { maxMemoryBudgetBytes_ = bytes; }

    // Evict least recently used clips to free memory
    int evictLRU(int64_t targetFreeBytes);

    // Update access time for a clip (call when clip is played)
    void touchClip(const std::string& clipId);

    // Statistics
    size_t getRegisteredCount() const;
    size_t getLoadedCount() const;
    size_t getPinnedCount() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ManagedClip> clips_;
    int64_t maxMemoryBudgetBytes_ = 256 * 1024 * 1024;  // 256 MB default

    // Estimate memory usage for a clip
    int64_t estimateMemoryUsage(const VideoClip& clip) const;
};

} // namespace video
} // namespace oblivion
