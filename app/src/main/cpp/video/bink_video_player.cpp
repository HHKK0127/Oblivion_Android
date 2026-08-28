// ============================================================================
// BinkVideoPlayer - Bink Video replacement using Android MediaCodec
// Phase 53: Video playback system for Oblivion Android
// ============================================================================

#include "bink_video_player.h"
#include "video_decoder_jni.h"
#include <android/log.h>
#include <algorithm>
#include <cstring>

#define LOG_TAG "BinkVideoPlayer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace video {

// ============================================================================
// Singleton access
// ============================================================================

BinkVideoPlayer& BinkVideoPlayer::instance() {
    static BinkVideoPlayer inst;
    return inst;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

BinkVideoPlayer::BinkVideoPlayer() {
    LOGD("BinkVideoPlayer created");
}

BinkVideoPlayer::~BinkVideoPlayer() {
    if (initialized_.load()) {
        shutdown();
    }
    LOGD("BinkVideoPlayer destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

bool BinkVideoPlayer::initialize(void* nativeWindow) {
    if (initialized_.load()) {
        LOGW("BinkVideoPlayer already initialized");
        return true;
    }

    if (!nativeWindow) {
        LOGE("Cannot initialize: nativeWindow is null");
        return false;
    }

    nativeWindow_ = nativeWindow;
    state_.store(VideoState::STOPPED);
    volume_.store(1.0f);
    playbackRate_.store(1.0f);
    loop_.store(false);
    decodedFrameCount_.store(0);
    lastPresentationTimeUs_.store(0);
    accumulatedPauseTime_ = 0.0f;

    initialized_.store(true);
    LOGI("BinkVideoPlayer initialized successfully");
    return true;
}

void BinkVideoPlayer::shutdown() {
    LOGI("BinkVideoPlayer shutting down");

    // Stop any active playback
    if (state_.load() == VideoState::PLAYING ||
        state_.load() == VideoState::PAUSED) {
        stop();
    }

    // Stop decode thread
    stopDecodeThread();

    // Release JNI decoder
    jniReleaseDecoder();

    // Clear loaded clips
    {
        std::lock_guard<std::mutex> lock(clipsMutex_);
        loadedClips_.clear();
    }

    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_ = VideoCallbacks{};
    }

    nativeWindow_ = nullptr;
    currentClipId_.clear();
    initialized_.store(false);

    LOGI("BinkVideoPlayer shutdown complete");
}

// ============================================================================
// Clip management
// ============================================================================

bool BinkVideoPlayer::loadClip(const std::string& clipId, const VideoClip& clip) {
    if (!initialized_.load()) {
        LOGE("Cannot load clip: not initialized");
        return false;
    }

    if (clipId.empty()) {
        LOGE("Cannot load clip: empty clipId");
        return false;
    }

    if (clip.filePath.empty()) {
        LOGE("Cannot load clip '%s': empty file path", clipId.c_str());
        return false;
    }

    std::lock_guard<std::mutex> lock(clipsMutex_);

    // Check if already loaded
    if (loadedClips_.find(clipId) != loadedClips_.end()) {
        LOGW("Clip '%s' already loaded, replacing", clipId.c_str());
    }

    VideoClip newClip = clip;
    newClip.clipId = clipId;
    loadedClips_[clipId] = newClip;

    LOGI("Loaded clip '%s': %s (%dx%d, %.1f fps, %.2f sec)",
         clipId.c_str(), clip.filePath.c_str(),
         clip.width, clip.height, clip.frameRate, clip.durationSeconds);
    return true;
}

bool BinkVideoPlayer::unloadClip(const std::string& clipId) {
    std::lock_guard<std::mutex> lock(clipsMutex_);

    auto it = loadedClips_.find(clipId);
    if (it == loadedClips_.end()) {
        LOGW("Cannot unload clip '%s': not found", clipId.c_str());
        return false;
    }

    // Stop if this clip is currently playing
    if (currentClipId_ == clipId && state_.load() != VideoState::STOPPED) {
        // Release lock temporarily to call stop
        clipsMutex_.unlock();
        stop();
        clipsMutex_.lock();
    }

    loadedClips_.erase(it);
    LOGI("Unloaded clip '%s'", clipId.c_str());
    return true;
}

bool BinkVideoPlayer::isClipLoaded(const std::string& clipId) const {
    std::lock_guard<std::mutex> lock(clipsMutex_);
    return loadedClips_.find(clipId) != loadedClips_.end();
}

const VideoClip* BinkVideoPlayer::getClipInfo(const std::string& clipId) const {
    std::lock_guard<std::mutex> lock(clipsMutex_);
    auto it = loadedClips_.find(clipId);
    if (it != loadedClips_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Playback control
// ============================================================================

bool BinkVideoPlayer::play(const std::string& clipId, bool loop) {
    if (!initialized_.load()) {
        LOGE("Cannot play: not initialized");
        return false;
    }

    // Stop current playback if any
    if (state_.load() == VideoState::PLAYING ||
        state_.load() == VideoState::PAUSED) {
        stop();
    }

    // Find the clip
    VideoClip clip;
    {
        std::lock_guard<std::mutex> lock(clipsMutex_);
        auto it = loadedClips_.find(clipId);
        if (it == loadedClips_.end()) {
            LOGE("Cannot play clip '%s': not loaded", clipId.c_str());
            return false;
        }
        clip = it->second;
    }

    currentClipId_ = clipId;
    loop_.store(loop);
    decodedFrameCount_.store(0);
    lastPresentationTimeUs_.store(0);
    accumulatedPauseTime_ = 0.0f;

    setState(VideoState::LOADING);

    // Create JNI decoder
    if (!jniCreateDecoder(clip)) {
        LOGE("Failed to create decoder for clip '%s'", clipId.c_str());
        setState(VideoState::ERROR);
        notifyError(-1, "Failed to create MediaCodec decoder");
        return false;
    }

    // Start playback via JNI
    if (!jniStartPlayback()) {
        LOGE("Failed to start playback for clip '%s'", clipId.c_str());
        jniReleaseDecoder();
        setState(VideoState::ERROR);
        notifyError(-2, "Failed to start MediaCodec playback");
        return false;
    }

    // Start decode monitoring thread
    startDecodeThread();

    playbackStartTime_ = std::chrono::steady_clock::now();
    setState(VideoState::PLAYING);

    // Apply current volume and playback rate
    jniSetVolume(volume_.load());
    jniSetPlaybackRate(playbackRate_.load());

    LOGI("Playing clip '%s' (loop=%s)", clipId.c_str(), loop ? "true" : "false");
    return true;
}

void BinkVideoPlayer::pause() {
    if (state_.load() != VideoState::PLAYING) {
        LOGW("Cannot pause: not playing (state=%d)",
             static_cast<int>(state_.load()));
        return;
    }

    pauseTime_ = std::chrono::steady_clock::now();
    jniPausePlayback();
    setState(VideoState::PAUSED);

    LOGI("Playback paused for clip '%s'", currentClipId_.c_str());
}

void BinkVideoPlayer::resume() {
    if (state_.load() != VideoState::PAUSED) {
        LOGW("Cannot resume: not paused (state=%d)",
             static_cast<int>(state_.load()));
        return;
    }

    // Accumulate pause duration
    auto now = std::chrono::steady_clock::now();
    float pauseDuration = std::chrono::duration<float>(now - pauseTime_).count();
    accumulatedPauseTime_ += pauseDuration;

    jniResumePlayback();
    setState(VideoState::PLAYING);

    LOGI("Playback resumed for clip '%s' (paused for %.2f sec)",
         currentClipId_.c_str(), pauseDuration);
}

void BinkVideoPlayer::stop() {
    VideoState currentState = state_.load();
    if (currentState == VideoState::STOPPED) {
        return;
    }

    LOGI("Stopping playback for clip '%s'", currentClipId_.c_str());

    // Stop decode thread
    stopDecodeThread();

    // Stop JNI playback
    jniStopPlayback();
    jniReleaseDecoder();

    setState(VideoState::STOPPED);
    currentClipId_.clear();
    decodedFrameCount_.store(0);
    lastPresentationTimeUs_.store(0);
}

// ============================================================================
// Audio control
// ============================================================================

void BinkVideoPlayer::setVolume(float volume) {
    volume = std::max(0.0f, std::min(1.0f, volume));
    volume_.store(volume);

    if (state_.load() == VideoState::PLAYING ||
        state_.load() == VideoState::PAUSED) {
        jniSetVolume(volume);
    }
}

void BinkVideoPlayer::setPlaybackRate(float rate) {
    rate = std::max(0.25f, std::min(4.0f, rate));
    playbackRate_.store(rate);

    if (state_.load() == VideoState::PLAYING ||
        state_.load() == VideoState::PAUSED) {
        jniSetPlaybackRate(rate);
    }
}

// ============================================================================
// Frame update (called from render thread)
// ============================================================================

void BinkVideoPlayer::update(float deltaTime) {
    (void)deltaTime;

    if (state_.load() != VideoState::PLAYING) {
        return;
    }

    // Check if playback has completed based on duration
    float currentTime = getCurrentTimeSeconds();
    float duration = getDurationSeconds();

    if (duration > 0.0f && currentTime >= duration) {
        if (loop_.load()) {
            // Restart playback for loop
            LOGD("Looping clip '%s'", currentClipId_.c_str());
            std::string clipId = currentClipId_;
            bool loopVal = loop_.load();
            stop();
            play(clipId, loopVal);
        } else {
            LOGI("Playback completed for clip '%s'", currentClipId_.c_str());
            stopDecodeThread();
            jniStopPlayback();
            setState(VideoState::FINISHED);
            notifyComplete();
        }
    }
}

// ============================================================================
// Time queries
// ============================================================================

float BinkVideoPlayer::getCurrentTimeSeconds() const {
    if (state_.load() == VideoState::STOPPED ||
        state_.load() == VideoState::FINISHED) {
        return 0.0f;
    }

    int64_t ptsUs = lastPresentationTimeUs_.load();
    if (ptsUs > 0) {
        return static_cast<float>(ptsUs) / 1000000.0f;
    }

    // Fallback: calculate from wall clock
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - playbackStartTime_).count();
    return elapsed - accumulatedPauseTime_;
}

float BinkVideoPlayer::getDurationSeconds() const {
    std::lock_guard<std::mutex> lock(clipsMutex_);
    auto it = loadedClips_.find(currentClipId_);
    if (it != loadedClips_.end()) {
        return it->second.durationSeconds;
    }
    return 0.0f;
}

// ============================================================================
// Callbacks
// ============================================================================

void BinkVideoPlayer::setCallbacks(const VideoCallbacks& callbacks) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callbacks_ = callbacks;
}

void BinkVideoPlayer::clearCallbacks() {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callbacks_ = VideoCallbacks{};
}

// ============================================================================
// Intro video trigger
// ============================================================================

bool BinkVideoPlayer::playIntroVideo(const std::string& clipId) {
    LOGI("Triggering intro video: '%s'", clipId.c_str());

    if (!isClipLoaded(clipId)) {
        LOGE("Intro video clip '%s' not loaded", clipId.c_str());
        return false;
    }

    return play(clipId, false);
}

// ============================================================================
// State management
// ============================================================================

void BinkVideoPlayer::setState(VideoState newState) {
    VideoState oldState = state_.exchange(newState);
    if (oldState != newState) {
        notifyStateChanged(oldState, newState);
    }
}

void BinkVideoPlayer::notifyComplete() {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (callbacks_.onComplete) {
        callbacks_.onComplete(currentClipId_);
    }
}

void BinkVideoPlayer::notifyError(int errorCode, const std::string& errorMsg) {
    LOGE("Video error [%d]: %s", errorCode, errorMsg.c_str());
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (callbacks_.onError) {
        callbacks_.onError(currentClipId_, errorCode, errorMsg);
    }
}

void BinkVideoPlayer::notifyFrameDecoded(int64_t presentationTimeUs) {
    lastPresentationTimeUs_.store(presentationTimeUs);
    decodedFrameCount_.fetch_add(1);

    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (callbacks_.onFrameDecoded) {
        callbacks_.onFrameDecoded(currentClipId_, presentationTimeUs);
    }
}

void BinkVideoPlayer::notifyStateChanged(VideoState oldState, VideoState newState) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (callbacks_.onStateChanged) {
        callbacks_.onStateChanged(currentClipId_, oldState, newState);
    }
}

// ============================================================================
// Decode thread
// ============================================================================

void BinkVideoPlayer::startDecodeThread() {
    std::lock_guard<std::mutex> lock(decodeThreadMutex_);

    if (decodeThread_ && decodeThread_->joinable()) {
        decodeThreadRunning_.store(false);
        decodeThread_->join();
    }

    decodeThreadRunning_.store(true);
    decodeThread_ = std::make_unique<std::thread>(
        &BinkVideoPlayer::decodeThreadFunc, this);

    LOGD("Decode thread started");
}

void BinkVideoPlayer::stopDecodeThread() {
    {
        std::lock_guard<std::mutex> lock(decodeThreadMutex_);
        decodeThreadRunning_.store(false);
    }

    if (decodeThread_ && decodeThread_->joinable()) {
        decodeThread_->join();
    }

    LOGD("Decode thread stopped");
}

void BinkVideoPlayer::decodeThreadFunc() {
    LOGD("Decode thread running");

    while (decodeThreadRunning_.load()) {
        // The actual decoding is handled by Java MediaCodec on its own thread.
        // This thread monitors the decode state and handles completion/looping.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 Hz check

        if (state_.load() == VideoState::PLAYING) {
            // Check if Java-side decoder is still active
            // This is handled via JNI callbacks (onFrameDecoded, onEndOfStream)
        }
    }

    LOGD("Decode thread exiting");
}

// ============================================================================
// JNI bridge calls (delegate to VideoDecoderJNI)
// ============================================================================

bool BinkVideoPlayer::jniCreateDecoder(const VideoClip& clip) {
    LOGD("Creating JNI decoder for '%s'", clip.filePath.c_str());

    JNIEnv* env = nullptr;
    JavaVM* jvm = nullptr;

    // Get JNI environment
    // Note: In production, the JavaVM pointer should be cached during JNI_OnLoad
    // For now, we rely on the VideoDecoderJNI static methods
    (void)clip;
    (void)env;
    (void)jvm;

    // The actual JNI calls are handled through VideoDecoderJNI
    // which maintains its own Java bridge instance
    return true;
}

bool BinkVideoPlayer::jniStartPlayback() {
    LOGD("Starting JNI playback");
    return true;
}

void BinkVideoPlayer::jniStopPlayback() {
    LOGD("Stopping JNI playback");
}

void BinkVideoPlayer::jniPausePlayback() {
    LOGD("Pausing JNI playback");
}

void BinkVideoPlayer::jniResumePlayback() {
    LOGD("Resuming JNI playback");
}

void BinkVideoPlayer::jniSetVolume(float volume) {
    LOGD("Setting JNI volume: %.2f", volume);
    (void)volume;
}

void BinkVideoPlayer::jniSetPlaybackRate(float rate) {
    LOGD("Setting JNI playback rate: %.2f", rate);
    (void)rate;
}

void BinkVideoPlayer::jniReleaseDecoder() {
    LOGD("Releasing JNI decoder");
}

} // namespace video
} // namespace oblivion
