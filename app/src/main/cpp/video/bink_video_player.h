#pragma once

// ============================================================================
// BinkVideoPlayer - Bink Video replacement using Android MediaCodec
// Phase 53: Video playback system for Oblivion Android
// ============================================================================

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <cstdint>
#include <vector>
#include <thread>
#include <chrono>

// Forward declarations
struct ANativeWindow;

namespace oblivion {
namespace video {

// ============================================================================
// Video clip metadata
// ============================================================================

struct VideoClip {
    std::string clipId;
    std::string filePath;
    int width = 0;
    int height = 0;
    float frameRate = 30.0f;
    float durationSeconds = 0.0f;
    int audioTrackCount = 0;
    bool hasAudio = false;
    std::string codecType;  // e.g., "video/avc", "video/hevc"
    int64_t fileSizeBytes = 0;
};

// ============================================================================
// Video playback state
// ============================================================================

enum class VideoState {
    STOPPED,
    LOADING,
    PLAYING,
    PAUSED,
    FINISHED,
    ERROR
};

// ============================================================================
// Video callback definitions
// ============================================================================

struct VideoCallbacks {
    // Called when video playback completes naturally
    std::function<void(const std::string& clipId)> onComplete;

    // Called on decode or playback error
    std::function<void(const std::string& clipId, int errorCode,
                       const std::string& errorMsg)> onError;

    // Called each time a new frame is decoded and ready for display
    std::function<void(const std::string& clipId, int64_t presentationTimeUs)> onFrameDecoded;

    // Called when video state changes
    std::function<void(const std::string& clipId, VideoState oldState,
                       VideoState newState)> onStateChanged;
};

// ============================================================================
// BinkVideoPlayer - singleton video playback manager
// ============================================================================

class BinkVideoPlayer {
public:
    static BinkVideoPlayer& instance();

    // Initialization and shutdown
    bool initialize(void* nativeWindow);
    void shutdown();
    bool isInitialized() const { return initialized_.load(); }

    // Clip management
    bool loadClip(const std::string& clipId, const VideoClip& clip);
    bool unloadClip(const std::string& clipId);
    bool isClipLoaded(const std::string& clipId) const;
    const VideoClip* getClipInfo(const std::string& clipId) const;

    // Playback control
    bool play(const std::string& clipId, bool loop = false);
    void pause();
    void resume();
    void stop();

    // Playback state
    VideoState getState() const { return state_.load(); }
    std::string getCurrentClipId() const { return currentClipId_; }
    float getCurrentTimeSeconds() const;
    float getDurationSeconds() const;
    bool isPlaying() const { return state_.load() == VideoState::PLAYING; }
    bool isPaused() const { return state_.load() == VideoState::PAUSED; }

    // Audio control
    void setVolume(float volume);
    float getVolume() const { return volume_.load(); }
    void setPlaybackRate(float rate);
    float getPlaybackRate() const { return playbackRate_.load(); }

    // Frame update (call from render thread)
    void update(float deltaTime);

    // Callbacks
    void setCallbacks(const VideoCallbacks& callbacks);
    void clearCallbacks();

    // Intro video trigger
    bool playIntroVideo(const std::string& clipId);

private:
    BinkVideoPlayer();
    ~BinkVideoPlayer();
    BinkVideoPlayer(const BinkVideoPlayer&) = delete;
    BinkVideoPlayer& operator=(const BinkVideoPlayer&) = delete;

    // State management
    void setState(VideoState newState);
    void notifyComplete();
    void notifyError(int errorCode, const std::string& errorMsg);
    void notifyFrameDecoded(int64_t presentationTimeUs);
    void notifyStateChanged(VideoState oldState, VideoState newState);

    // Decode thread management
    void startDecodeThread();
    void stopDecodeThread();
    void decodeThreadFunc();

    // JNI bridge calls
    bool jniCreateDecoder(const VideoClip& clip);
    bool jniStartPlayback();
    void jniStopPlayback();
    void jniPausePlayback();
    void jniResumePlayback();
    void jniSetVolume(float volume);
    void jniSetPlaybackRate(float rate);
    void jniReleaseDecoder();

    // Member variables
    std::atomic<bool> initialized_{false};
    std::atomic<VideoState> state_{VideoState::STOPPED};
    std::atomic<float> volume_{1.0f};
    std::atomic<float> playbackRate_{1.0f};
    std::atomic<bool> loop_{false};
    std::atomic<bool> decodeThreadRunning_{false};

    std::string currentClipId_;
    void* nativeWindow_ = nullptr;

    mutable std::mutex clipsMutex_;
    std::unordered_map<std::string, VideoClip> loadedClips_;

    mutable std::mutex callbackMutex_;
    VideoCallbacks callbacks_;

    std::mutex decodeThreadMutex_;
    std::unique_ptr<std::thread> decodeThread_;

    // Timing
    std::chrono::steady_clock::time_point playbackStartTime_;
    std::chrono::steady_clock::time_point pauseTime_;
    float accumulatedPauseTime_ = 0.0f;

    // Frame tracking
    std::atomic<int64_t> lastPresentationTimeUs_{0};
    std::atomic<int> decodedFrameCount_{0};
};

} // namespace video
} // namespace oblivion
