// ============================================================================
// VideoClipManager - manages video clip preloading and memory
// Phase 53: Bink Video replacement
// ============================================================================

#include "video_clip_manager.h"
#include <android/log.h>
#include <algorithm>
#include <cstring>

#define LOG_TAG "VideoClipManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace video {

// ============================================================================
// Constructor / Destructor
// ============================================================================

VideoClipManager::VideoClipManager() {
    LOGD("VideoClipManager created (budget: %lu bytes)",
         static_cast<unsigned long>(maxMemoryBudgetBytes_));
}

VideoClipManager::~VideoClipManager() {
    unloadAll();
    LOGD("VideoClipManager destroyed");
}

// ============================================================================
// Clip registration
// ============================================================================

bool VideoClipManager::registerClip(const VideoClip& clip) {
    if (clip.clipId.empty()) {
        LOGE("Cannot register clip: empty clipId");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (clips_.find(clip.clipId) != clips_.end()) {
        LOGW("Clip '%s' already registered, updating", clip.clipId.c_str());
    }

    ManagedClip managed;
    managed.clip = clip;
    managed.loadState = ClipLoadState::UNLOADED;
    managed.refCount = 0;
    managed.memoryUsageBytes = 0;
    managed.lastAccessTime = std::chrono::steady_clock::now();
    managed.pinned = false;

    clips_[clip.clipId] = managed;

    LOGI("Registered clip '%s': %s (%dx%d, %.2f sec)",
         clip.clipId.c_str(), clip.filePath.c_str(),
         clip.width, clip.height, clip.durationSeconds);
    return true;
}

// ============================================================================
// Preloading
// ============================================================================

bool VideoClipManager::preloadClip(const std::string& clipId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = clips_.find(clipId);
    if (it == clips_.end()) {
        LOGE("Cannot preload clip '%s': not registered", clipId.c_str());
        return false;
    }

    ManagedClip& managed = it->second;

    if (managed.loadState == ClipLoadState::LOADED) {
        LOGD("Clip '%s' already preloaded", clipId.c_str());
        managed.lastAccessTime = std::chrono::steady_clock::now();
        return true;
    }

    if (managed.loadState == ClipLoadState::LOADING) {
        LOGW("Clip '%s' is currently loading", clipId.c_str());
        return false;
    }

    managed.loadState = ClipLoadState::LOADING;

    // Estimate memory usage
    int64_t estimatedBytes = estimateMemoryUsage(managed.clip);

    // Check if we need to evict clips to make room
    int64_t currentUsage = getTotalMemoryUsage();
    int64_t availableBudget = maxMemoryBudgetBytes_ - currentUsage;

    if (estimatedBytes > availableBudget) {
        int64_t needBytes = estimatedBytes - availableBudget;
        int evicted = evictLRU(needBytes);
        LOGD("Evicted %d clips to free %lu bytes for '%s'",
             evicted, static_cast<unsigned long>(needBytes), clipId.c_str());
    }

    // In a real implementation, this would:
    // 1. Open the video file and read container metadata
    // 2. Parse codec information
    // 3. Pre-allocate decode buffers
    // For now, we simulate the preload by marking as loaded
    managed.memoryUsageBytes = estimatedBytes;
    managed.loadState = ClipLoadState::LOADED;
    managed.lastAccessTime = std::chrono::steady_clock::now();

    LOGI("Preloaded clip '%s' (%lu bytes estimated)",
         clipId.c_str(), static_cast<unsigned long>(estimatedBytes));
    return true;
}

// ============================================================================
// Unloading
// ============================================================================

bool VideoClipManager::unloadClip(const std::string& clipId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = clips_.find(clipId);
    if (it == clips_.end()) {
        LOGW("Cannot unload clip '%s': not registered", clipId.c_str());
        return false;
    }

    ManagedClip& managed = it->second;

    if (managed.loadState != ClipLoadState::LOADED) {
        LOGD("Clip '%s' not loaded, nothing to unload", clipId.c_str());
        return true;
    }

    if (managed.refCount > 0) {
        LOGW("Unloading clip '%s' with %d active references",
             clipId.c_str(), managed.refCount);
    }

    managed.loadState = ClipLoadState::UNLOADED;
    managed.memoryUsageBytes = 0;

    LOGI("Unloaded clip '%s'", clipId.c_str());
    return true;
}

void VideoClipManager::unloadAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t unloadedCount = 0;
    for (auto& pair : clips_) {
        if (pair.second.loadState == ClipLoadState::LOADED) {
            pair.second.loadState = ClipLoadState::UNLOADED;
            pair.second.memoryUsageBytes = 0;
            unloadedCount++;
        }
    }

    LOGI("Unloaded all clips (%lu clips)", static_cast<unsigned long>(unloadedCount));
}

// ============================================================================
// Pin management
// ============================================================================

bool VideoClipManager::pinClip(const std::string& clipId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = clips_.find(clipId);
    if (it == clips_.end()) {
        LOGW("Cannot pin clip '%s': not registered", clipId.c_str());
        return false;
    }

    it->second.pinned = true;
    LOGD("Pinned clip '%s'", clipId.c_str());
    return true;
}

bool VideoClipManager::unpinClip(const std::string& clipId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = clips_.find(clipId);
    if (it == clips_.end()) {
        return false;
    }

    it->second.pinned = false;
    LOGD("Unpinned clip '%s'", clipId.c_str());
    return true;
}

// ============================================================================
// Query
// ============================================================================

const ManagedClip* VideoClipManager::getManagedClip(const std::string& clipId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = clips_.find(clipId);
    if (it != clips_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> VideoClipManager::getRegisteredClipIds() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> ids;
    ids.reserve(clips_.size());
    for (const auto& pair : clips_) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::string> VideoClipManager::getLoadedClipIds() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> ids;
    for (const auto& pair : clips_) {
        if (pair.second.loadState == ClipLoadState::LOADED) {
            ids.push_back(pair.first);
        }
    }
    return ids;
}

// ============================================================================
// Memory management
// ============================================================================

int64_t VideoClipManager::getTotalMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);

    int64_t total = 0;
    for (const auto& pair : clips_) {
        total += pair.second.memoryUsageBytes;
    }
    return total;
}

int VideoClipManager::evictLRU(int64_t targetFreeBytes) {
    // Note: caller must hold mutex_

    // Collect non-pinned, loaded clips sorted by last access time
    struct EvictCandidate {
        std::string clipId;
        std::chrono::steady_clock::time_point lastAccess;
        int64_t memoryBytes;
    };

    std::vector<EvictCandidate> candidates;
    for (const auto& pair : clips_) {
        if (pair.second.loadState == ClipLoadState::LOADED &&
            !pair.second.pinned &&
            pair.second.refCount == 0) {
            candidates.push_back({
                pair.first,
                pair.second.lastAccessTime,
                pair.second.memoryUsageBytes
            });
        }
    }

    // Sort by access time (oldest first)
    std::sort(candidates.begin(), candidates.end(),
              [](const EvictCandidate& a, const EvictCandidate& b) {
                  return a.lastAccess < b.lastAccess;
              });

    int evictedCount = 0;
    int64_t freedBytes = 0;

    for (const auto& candidate : candidates) {
        if (freedBytes >= targetFreeBytes) {
            break;
        }

        auto it = clips_.find(candidate.clipId);
        if (it != clips_.end()) {
            it->second.loadState = ClipLoadState::UNLOADED;
            it->second.memoryUsageBytes = 0;
            freedBytes += candidate.memoryBytes;
            evictedCount++;
        }
    }

    if (evictedCount > 0) {
        LOGD("LRU eviction: freed %lu bytes by unloading %d clips",
             static_cast<unsigned long>(freedBytes), evictedCount);
    }

    return evictedCount;
}

void VideoClipManager::touchClip(const std::string& clipId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = clips_.find(clipId);
    if (it != clips_.end()) {
        it->second.lastAccessTime = std::chrono::steady_clock::now();
    }
}

// ============================================================================
// Statistics
// ============================================================================

size_t VideoClipManager::getRegisteredCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clips_.size();
}

size_t VideoClipManager::getLoadedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& pair : clips_) {
        if (pair.second.loadState == ClipLoadState::LOADED) {
            count++;
        }
    }
    return count;
}

size_t VideoClipManager::getPinnedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& pair : clips_) {
        if (pair.second.pinned) {
            count++;
        }
    }
    return count;
}

// ============================================================================
// Memory estimation
// ============================================================================

int64_t VideoClipManager::estimateMemoryUsage(const VideoClip& clip) const {
    // Rough estimate: width * height * 4 bytes (RGBA) * buffered frames
    // Assume ~10 frames buffered for smooth playback
    constexpr int kBufferedFrames = 10;
    int64_t frameBytes = static_cast<int64_t>(clip.width) *
                         static_cast<int64_t>(clip.height) * 4;
    int64_t totalBytes = frameBytes * kBufferedFrames;

    // Add audio buffer estimate (stereo 16-bit at 44100 Hz, ~1 sec buffer)
    if (clip.hasAudio) {
        totalBytes += 44100 * 2 * 2;  // ~176 KB for 1 sec audio
    }

    // Minimum 1 MB
    if (totalBytes < 1024 * 1024) {
        totalBytes = 1024 * 1024;
    }

    return totalBytes;
}

} // namespace video
} // namespace oblivion
