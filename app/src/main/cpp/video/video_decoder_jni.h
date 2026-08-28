#pragma once

// ============================================================================
// VideoDecoderJNI - JNI bridge for Android MediaCodec video decoding
// Phase 53: Bink Video replacement
// ============================================================================

#include <jni.h>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace oblivion {
namespace video {

// ============================================================================
// JNI callback interface for decoded frames
// ============================================================================

class IVideoDecoderCallback {
public:
    virtual ~IVideoDecoderCallback() = default;

    // Called when a frame is decoded and available on the output surface
    virtual void onFrameAvailable(int64_t presentationTimeUs) = 0;

    // Called when decoding reaches end of stream
    virtual void onEndOfStream() = 0;

    // Called on decode error
    virtual void onDecodeError(int errorCode, const std::string& message) = 0;

    // Called when decoder is ready
    virtual void onDecoderReady(int width, int height, float frameRate) = 0;
};

// ============================================================================
// VideoDecoderJNI - manages JNI communication with Java VideoBridge
// ============================================================================

class VideoDecoderJNI {
public:
    VideoDecoderJNI();
    ~VideoDecoderJNI();

    // Initialize JNI references (call once on library load)
    static bool initJNI(JNIEnv* env);

    // Set callback for decoded frames
    void setCallback(IVideoDecoderCallback* callback);

    // Create a MediaCodec decoder for the given file path
    bool createDecoder(JNIEnv* env, const std::string& filePath,
                       void* nativeWindow);

    // Start the asynchronous decode loop
    bool startDecoding(JNIEnv* env);

    // Pause decoding
    void pauseDecoding(JNIEnv* env);

    // Resume decoding
    void resumeDecoding(JNIEnv* env);

    // Stop decoding and release resources
    void stopDecoding(JNIEnv* env);

    // Set volume for audio track
    void setVolume(JNIEnv* env, float volume);

    // Set playback rate
    void setPlaybackRate(JNIEnv* env, float rate);

    // Check if decoder is active
    bool isActive() const { return active_.load(); }

    // Get decoded frame count
    int getDecodedFrameCount() const { return decodedFrameCount_.load(); }

    // Get current presentation time in microseconds
    int64_t getCurrentPresentationTimeUs() const { return currentPtsUs_.load(); }

    // JNI callback methods (called from Java)
    static void JNICALL nativeOnFrameDecoded(JNIEnv* env, jclass clazz,
                                              jlong decoderHandle,
                                              jlong presentationTimeUs);

    static void JNICALL nativeOnEndOfStream(JNIEnv* env, jclass clazz,
                                             jlong decoderHandle);

    static void JNICALL nativeOnDecodeError(JNIEnv* env, jclass clazz,
                                             jlong decoderHandle,
                                             jint errorCode,
                                             jstring message);

    static void JNICALL nativeOnDecoderReady(JNIEnv* env, jclass clazz,
                                              jlong decoderHandle,
                                              jint width, jint height,
                                              jfloat frameRate);

private:
    // JNI class and method references
    static jclass videoBridgeClass_;
    static jmethodID createDecoderMethod_;
    static jmethodID startDecodingMethod_;
    static jmethodID pauseDecodingMethod_;
    static jmethodID resumeDecodingMethod_;
    static jmethodID stopDecodingMethod_;
    static jmethodID setVolumeMethod_;
    static jmethodID setPlaybackRateMethod_;
    static jmethodID releaseMethod_;
    static bool jniInitialized_;

    // Java VideoBridge instance
    jobject javaBridge_ = nullptr;

    // Callback
    IVideoDecoderCallback* callback_ = nullptr;

    // State
    std::atomic<bool> active_{false};
    std::atomic<int> decodedFrameCount_{0};
    std::atomic<int64_t> currentPtsUs_{0};

    mutable std::mutex callbackMutex_;
};

// ============================================================================
// JNI registration helper
// ============================================================================

bool registerVideoDecoderNatives(JNIEnv* env);

} // namespace video
} // namespace oblivion
