#include "jni_audio_bridge.h"
#include <jni.h>
#include <android/log.h>
#include <mutex>

#define LOG_TAG "AudioJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Cached JNI objects
static JavaVM* g_jvm = nullptr;
static jclass g_main_activity_class = nullptr;
static jmethodID g_play_bgm_method = nullptr;
static jmethodID g_play_se_method = nullptr;
static jmethodID g_stop_bgm_method = nullptr;
static std::mutex g_jni_mutex;

class JNIEnvGuard {
    JavaVM* vm_;
    bool attached_;
public:
    explicit JNIEnvGuard(JavaVM* vm) : vm_(vm), attached_(false) {}
    ~JNIEnvGuard() {
        if (attached_ && vm_) {
            vm_->DetachCurrentThread();
        }
    }

    JNIEnv* getEnv() {
        if (!vm_) return nullptr;
        JNIEnv* env = nullptr;
        int status = vm_->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            if (vm_->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                attached_ = true;
            } else {
                LOGE("Failed to attach current thread to JVM");
                return nullptr;
            }
        } else if (status != JNI_OK) {
            LOGE("JNI GetEnv failed: %d", status);
            return nullptr;
        }
        return env;
    }
};

static void clear_pending_exception(JNIEnv* env, const char* operation) {
    if (env->ExceptionCheck()) {
        LOGE("JNI exception during %s", operation);
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void jni_audio_bridge_init(JavaVM* vm) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);

    if (g_jvm != nullptr) {
        LOGD("JNI audio bridge already initialized");
        return;
    }

    g_jvm = vm;

    // Get JNIEnv for method lookup
    JNIEnvGuard envGuard(g_jvm);
    JNIEnv* env = envGuard.getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv in jni_audio_bridge_init");
        return;
    }

    // Find MainActivity class
    jclass localClass = env->FindClass("com/example/oblivion/MainActivity");
    if (!localClass) {
        // FindClass failed - clear pending exception before returning
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        LOGE("Failed to find MainActivity class");
        return;
    }

    // Create global reference to keep class loaded
    g_main_activity_class = (jclass)env->NewGlobalRef(localClass);
    env->DeleteLocalRef(localClass);

    if (!g_main_activity_class) {
        LOGE("Failed to create global ref for MainActivity");
        return;
    }

    // Cache method IDs for static methods
    // public static void playBGM(String filename)
    g_play_bgm_method = env->GetStaticMethodID(g_main_activity_class, "playBGM", "(Ljava/lang/String;)V");
    if (!g_play_bgm_method) {
        clear_pending_exception(env, "playBGM method lookup");
        LOGE("Failed to find playBGM method");
        return;
    }

    // public static void playSE(String filename)
    g_play_se_method = env->GetStaticMethodID(g_main_activity_class, "playSE", "(Ljava/lang/String;)V");
    if (!g_play_se_method) {
        clear_pending_exception(env, "playSE method lookup");
        LOGE("Failed to find playSE method");
        return;
    }

    // public static void stopBGM()
    g_stop_bgm_method = env->GetStaticMethodID(g_main_activity_class, "stopBGM", "()V");
    if (!g_stop_bgm_method) {
        clear_pending_exception(env, "stopBGM method lookup");
        LOGE("Failed to find stopBGM method");
        return;
    }

    LOGI("JNI audio bridge initialized successfully");
}

void jni_audio_play_bgm(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);

    if (!g_jvm || !g_main_activity_class || !g_play_bgm_method) {
        LOGE("JNI audio bridge not properly initialized");
        return;
    }

    JNIEnvGuard envGuard(g_jvm);
    JNIEnv* env = envGuard.getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv for playBGM");
        return;
    }

    // Convert C++ string to jstring
    jstring javaFilename = env->NewStringUTF(filename.c_str());
    if (!javaFilename) {
        LOGE("Failed to create jstring for filename: %s", filename.c_str());
        return;
    }

    // Call static method: MainActivity.playBGM(filename)
    env->CallStaticVoidMethod(g_main_activity_class, g_play_bgm_method, javaFilename);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        LOGE("Exception in playBGM");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    // Clean up
    env->DeleteLocalRef(javaFilename);
    LOGD("Called playBGM with filename: %s", filename.c_str());
}

void jni_audio_play_se(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);

    if (!g_jvm || !g_main_activity_class || !g_play_se_method) {
        LOGE("JNI audio bridge not properly initialized");
        return;
    }

    JNIEnvGuard envGuard(g_jvm);
    JNIEnv* env = envGuard.getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv for playSE");
        return;
    }

    // Convert C++ string to jstring
    jstring javaFilename = env->NewStringUTF(filename.c_str());
    if (!javaFilename) {
        LOGE("Failed to create jstring for filename: %s", filename.c_str());
        return;
    }

    // Call static method: MainActivity.playSE(filename)
    env->CallStaticVoidMethod(g_main_activity_class, g_play_se_method, javaFilename);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        LOGE("Exception in playSE");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    // Clean up
    env->DeleteLocalRef(javaFilename);
    LOGD("Called playSE with filename: %s", filename.c_str());
}

void jni_audio_stop_bgm() {
    std::lock_guard<std::mutex> lock(g_jni_mutex);

    if (!g_jvm || !g_main_activity_class || !g_stop_bgm_method) {
        LOGE("JNI audio bridge not properly initialized");
        return;
    }

    JNIEnvGuard envGuard(g_jvm);
    JNIEnv* env = envGuard.getEnv();
    if (!env) {
        LOGE("Failed to get JNIEnv for stopBGM");
        return;
    }

    // Call static method: MainActivity.stopBGM()
    env->CallStaticVoidMethod(g_main_activity_class, g_stop_bgm_method);

    // Check for exceptions
    if (env->ExceptionCheck()) {
        LOGE("Exception in stopBGM");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    LOGD("Called stopBGM");
}

void jni_audio_bridge_cleanup() {
    std::lock_guard<std::mutex> lock(g_jni_mutex);

    if (!g_jvm) {
        return;
    }

    JNIEnvGuard envGuard(g_jvm);
    JNIEnv* env = envGuard.getEnv();
    if (env && g_main_activity_class) {
        env->DeleteGlobalRef(g_main_activity_class);
    }

    g_jvm = nullptr;
    g_main_activity_class = nullptr;
    g_play_bgm_method = nullptr;
    g_play_se_method = nullptr;
    g_stop_bgm_method = nullptr;

    LOGI("JNI audio bridge cleaned up");
}
