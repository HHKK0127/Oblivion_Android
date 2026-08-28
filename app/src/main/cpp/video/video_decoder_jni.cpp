// ============================================================================
// VideoDecoderJNI - JNI bridge for Android MediaCodec video decoding
// Phase 53: Bink Video replacement
// ============================================================================

#include "video_decoder_jni.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG "VideoDecoderJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace video {

// ============================================================================
// Static JNI references
// ============================================================================

jclass VideoDecoderJNI::videoBridgeClass_ = nullptr;
jmethodID VideoDecoderJNI::createDecoderMethod_ = nullptr;
jmethodID VideoDecoderJNI::startDecodingMethod_ = nullptr;
jmethodID VideoDecoderJNI::pauseDecodingMethod_ = nullptr;
jmethodID VideoDecoderJNI::resumeDecodingMethod_ = nullptr;
jmethodID VideoDecoderJNI::stopDecodingMethod_ = nullptr;
jmethodID VideoDecoderJNI::setVolumeMethod_ = nullptr;
jmethodID VideoDecoderJNI::setPlaybackRateMethod_ = nullptr;
jmethodID VideoDecoderJNI::releaseMethod_ = nullptr;
bool VideoDecoderJNI::jniInitialized_ = false;

// ============================================================================
// Constructor / Destructor
// ============================================================================

VideoDecoderJNI::VideoDecoderJNI() {
    LOGD("VideoDecoderJNI created");
}

VideoDecoderJNI::~VideoDecoderJNI() {
    LOGD("VideoDecoderJNI destroyed");
}

// ============================================================================
// JNI initialization (call once at library load)
// ============================================================================

bool VideoDecoderJNI::initJNI(JNIEnv* env) {
    if (jniInitialized_) {
        return true;
    }

    if (!env) {
        LOGE("initJNI: JNIEnv is null");
        return false;
    }

    // Find the VideoBridge Java class
    jclass localClass = env->FindClass("com/example/oblivion/VideoBridge");
    if (!localClass) {
        LOGE("initJNI: Failed to find VideoBridge class");
        return false;
    }

    // Create global reference to prevent GC
    videoBridgeClass_ = (jclass)env->NewGlobalRef(localClass);
    env->DeleteLocalRef(localClass);

    if (!videoBridgeClass_) {
        LOGE("initJNI: Failed to create global reference for VideoBridge");
        return false;
    }

    // Cache method IDs for performance
    createDecoderMethod_ = env->GetMethodID(
        videoBridgeClass_, "createDecoder",
        "(Ljava/lang/String;Landroid/view/Surface;)Z");
    if (!createDecoderMethod_) {
        LOGE("initJNI: Failed to find createDecoder method");
        return false;
    }

    startDecodingMethod_ = env->GetMethodID(
        videoBridgeClass_, "startDecoding", "()Z");
    if (!startDecodingMethod_) {
        LOGE("initJNI: Failed to find startDecoding method");
        return false;
    }

    pauseDecodingMethod_ = env->GetMethodID(
        videoBridgeClass_, "pauseDecoding", "()V");
    if (!pauseDecodingMethod_) {
        LOGE("initJNI: Failed to find pauseDecoding method");
        return false;
    }

    resumeDecodingMethod_ = env->GetMethodID(
        videoBridgeClass_, "resumeDecoding", "()V");
    if (!resumeDecodingMethod_) {
        LOGE("initJNI: Failed to find resumeDecoding method");
        return false;
    }

    stopDecodingMethod_ = env->GetMethodID(
        videoBridgeClass_, "stopDecoding", "()V");
    if (!stopDecodingMethod_) {
        LOGE("initJNI: Failed to find stopDecoding method");
        return false;
    }

    setVolumeMethod_ = env->GetMethodID(
        videoBridgeClass_, "setVolume", "(F)V");
    if (!setVolumeMethod_) {
        LOGE("initJNI: Failed to find setVolume method");
        return false;
    }

    setPlaybackRateMethod_ = env->GetMethodID(
        videoBridgeClass_, "setPlaybackRate", "(F)V");
    if (!setPlaybackRateMethod_) {
        LOGE("initJNI: Failed to find setPlaybackRate method");
        return false;
    }

    releaseMethod_ = env->GetMethodID(
        videoBridgeClass_, "release", "()V");
    if (!releaseMethod_) {
        LOGE("initJNI: Failed to find release method");
        return false;
    }

    jniInitialized_ = true;
    LOGI("VideoDecoderJNI: JNI references initialized successfully");
    return true;
}

// ============================================================================
// Callback management
// ============================================================================

void VideoDecoderJNI::setCallback(IVideoDecoderCallback* callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = callback;
}

// ============================================================================
// Decoder lifecycle
// ============================================================================

bool VideoDecoderJNI::createDecoder(JNIEnv* env, const std::string& filePath,
                                     void* nativeWindow) {
    if (!jniInitialized_ || !videoBridgeClass_) {
        LOGE("createDecoder: JNI not initialized");
        return false;
    }

    if (!env) {
        LOGE("createDecoder: JNIEnv is null");
        return false;
    }

    // Create Java VideoBridge instance
    jmethodID constructor = env->GetMethodID(videoBridgeClass_, "<init>", "()V");
    if (!constructor) {
        LOGE("createDecoder: Failed to find VideoBridge constructor");
        return false;
    }

    jobject localBridge = env->NewObject(videoBridgeClass_, constructor);
    if (!localBridge) {
        LOGE("createDecoder: Failed to create VideoBridge instance");
        return false;
    }

    // Store as global reference
    javaBridge_ = env->NewGlobalRef(localBridge);
    env->DeleteLocalRef(localBridge);

    if (!javaBridge_) {
        LOGE("createDecoder: Failed to create global reference");
        return false;
    }

    // Convert file path to Java string
    jstring jFilePath = env->NewStringUTF(filePath.c_str());
    if (!jFilePath) {
        LOGE("createDecoder: Failed to create Java string for file path");
        return false;
    }

    // Create Surface from native window (ANativeWindow)
    // Note: In production, we would create a Surface from ANativeWindow
    // For now, pass null and let Java side handle surface creation
    jobject jSurface = nullptr;

    // Call Java createDecoder
    jboolean result = env->CallBooleanMethod(
        javaBridge_, createDecoderMethod_, jFilePath, jSurface);

    env->DeleteLocalRef(jFilePath);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        LOGE("createDecoder: Java exception during createDecoder");
        return false;
    }

    if (!result) {
        LOGE("createDecoder: Java createDecoder returned false");
        return false;
    }

    LOGI("Decoder created for file: %s", filePath.c_str());
    return true;
}

bool VideoDecoderJNI::startDecoding(JNIEnv* env) {
    if (!javaBridge_ || !startDecodingMethod_) {
        LOGE("startDecoding: decoder not created");
        return false;
    }

    jboolean result = env->CallBooleanMethod(
        javaBridge_, startDecodingMethod_);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        LOGE("startDecoding: Java exception");
        return false;
    }

    if (result) {
        active_.store(true);
        LOGI("Decoding started");
    }

    return result;
}

void VideoDecoderJNI::pauseDecoding(JNIEnv* env) {
    if (!javaBridge_ || !pauseDecodingMethod_) {
        return;
    }

    env->CallVoidMethod(javaBridge_, pauseDecodingMethod_);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    LOGD("Decoding paused");
}

void VideoDecoderJNI::resumeDecoding(JNIEnv* env) {
    if (!javaBridge_ || !resumeDecodingMethod_) {
        return;
    }

    env->CallVoidMethod(javaBridge_, resumeDecodingMethod_);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    LOGD("Decoding resumed");
}

void VideoDecoderJNI::stopDecoding(JNIEnv* env) {
    if (!javaBridge_ || !stopDecodingMethod_) {
        return;
    }

    env->CallVoidMethod(javaBridge_, stopDecodingMethod_);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    active_.store(false);
    LOGD("Decoding stopped");
}

void VideoDecoderJNI::setVolume(JNIEnv* env, float volume) {
    if (!javaBridge_ || !setVolumeMethod_) {
        return;
    }

    env->CallVoidMethod(javaBridge_, setVolumeMethod_, (jfloat)volume);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void VideoDecoderJNI::setPlaybackRate(JNIEnv* env, float rate) {
    if (!javaBridge_ || !setPlaybackRateMethod_) {
        return;
    }

    env->CallVoidMethod(javaBridge_, setPlaybackRateMethod_, (jfloat)rate);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

// ============================================================================
// JNI callbacks (called from Java -> C++)
// ============================================================================

void JNICALL VideoDecoderJNI::nativeOnFrameDecoded(
    [[maybe_unused]] JNIEnv* env,
    [[maybe_unused]] jclass clazz,
    jlong decoderHandle,
    jlong presentationTimeUs) {

    auto* decoder = reinterpret_cast<VideoDecoderJNI*>(decoderHandle);
    if (!decoder) {
        return;
    }

    decoder->currentPtsUs_.store(presentationTimeUs);
    decoder->decodedFrameCount_.fetch_add(1);

    std::lock_guard<std::mutex> lock(decoder->callbackMutex_);
    if (decoder->callback_) {
        decoder->callback_->onFrameAvailable(presentationTimeUs);
    }
}

void JNICALL VideoDecoderJNI::nativeOnEndOfStream(
    [[maybe_unused]] JNIEnv* env,
    [[maybe_unused]] jclass clazz,
    jlong decoderHandle) {

    auto* decoder = reinterpret_cast<VideoDecoderJNI*>(decoderHandle);
    if (!decoder) {
        return;
    }

    decoder->active_.store(false);

    std::lock_guard<std::mutex> lock(decoder->callbackMutex_);
    if (decoder->callback_) {
        decoder->callback_->onEndOfStream();
    }

    LOGI("End of stream reached");
}

void JNICALL VideoDecoderJNI::nativeOnDecodeError(
    [[maybe_unused]] JNIEnv* env,
    [[maybe_unused]] jclass clazz,
    jlong decoderHandle,
    jint errorCode,
    jstring message) {

    auto* decoder = reinterpret_cast<VideoDecoderJNI*>(decoderHandle);
    if (!decoder) {
        return;
    }

    const char* msgStr = env->GetStringUTFChars(message, nullptr);
    std::string errorMsg = msgStr ? msgStr : "Unknown error";
    env->ReleaseStringUTFChars(message, msgStr);

    decoder->active_.store(false);

    std::lock_guard<std::mutex> lock(decoder->callbackMutex_);
    if (decoder->callback_) {
        decoder->callback_->onDecodeError(errorCode, errorMsg);
    }

    LOGE("Decode error [%d]: %s", errorCode, errorMsg.c_str());
}

void JNICALL VideoDecoderJNI::nativeOnDecoderReady(
    [[maybe_unused]] JNIEnv* env,
    [[maybe_unused]] jclass clazz,
    jlong decoderHandle,
    jint width, jint height,
    jfloat frameRate) {

    auto* decoder = reinterpret_cast<VideoDecoderJNI*>(decoderHandle);
    if (!decoder) {
        return;
    }

    std::lock_guard<std::mutex> lock(decoder->callbackMutex_);
    if (decoder->callback_) {
        decoder->callback_->onDecoderReady(width, height, frameRate);
    }

    LOGI("Decoder ready: %dx%d @ %.1f fps", width, height, frameRate);
}

// ============================================================================
// JNI native method registration
// ============================================================================

bool registerVideoDecoderNatives(JNIEnv* env) {
    if (!env) {
        return false;
    }

    static const JNINativeMethod methods[] = {
        {"nativeOnFrameDecoded", "(JJ)V",
         reinterpret_cast<void*>(&VideoDecoderJNI::nativeOnFrameDecoded)},
        {"nativeOnEndOfStream", "(J)V",
         reinterpret_cast<void*>(&VideoDecoderJNI::nativeOnEndOfStream)},
        {"nativeOnDecodeError", "(JILjava/lang/String;)V",
         reinterpret_cast<void*>(&VideoDecoderJNI::nativeOnDecodeError)},
        {"nativeOnDecoderReady", "(JIIF)V",
         reinterpret_cast<void*>(&VideoDecoderJNI::nativeOnDecoderReady)},
    };

    // Find the native bridge class for registration
    jclass bridgeClass = env->FindClass("com/example/oblivion/VideoBridge");
    if (!bridgeClass) {
        LOGE("registerVideoDecoderNatives: VideoBridge class not found");
        return false;
    }

    int result = env->RegisterNatives(bridgeClass, methods,
                                       sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(bridgeClass);

    if (result < 0) {
        LOGE("registerVideoDecoderNatives: RegisterNatives failed (%d)", result);
        return false;
    }

    LOGI("Video decoder native methods registered successfully");
    return true;
}

} // namespace video
} // namespace oblivion
