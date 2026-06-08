#include <android/asset_manager.h>
#include <android/log.h>
#include <jni.h>
#include <cstring>

#undef LOG_TAG
#define LOG_TAG "AudioJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern AAssetManager* g_assetManager;
JavaVM* g_javaVM = nullptr;
jobject g_mainActivity = nullptr;

// Method IDs for MainActivity audio methods (cached)
static jmethodID g_playBGMMethodId = nullptr;
static jmethodID g_stopBGMMethodId = nullptr;
static jmethodID g_playSEMethodId = nullptr;

extern "C" {

void jni_audio_set_asset_manager(AAssetManager* mgr) {
    g_assetManager = mgr;
    if (mgr) {
        LOGI("AAssetManager set for audio");
    }
}

AAssetManager* jni_audio_get_asset_manager() {
    return g_assetManager;
}

void jni_audio_set_java_vm(JavaVM* vm) {
    g_javaVM = vm;
    if (vm) {
        LOGI("JavaVM set for audio");
    }
}

void jni_audio_set_main_activity(jobject activity) {
    JNIEnv* env = nullptr;
    if (!g_javaVM) {
        LOGE("JavaVM not initialized");
        return;
    }

    g_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) {
        LOGE("Failed to get JNI environment");
        return;
    }

    // Previous activity reference の削除（参照カウント管理）
    if (g_mainActivity) {
        env->DeleteGlobalRef(g_mainActivity);
    }

    // 新しい activity への global reference を作成
    g_mainActivity = env->NewGlobalRef(activity);
    if (g_mainActivity) {
        LOGI("MainActivity reference set for audio");

        // Method IDs をキャッシュ
        jclass activityClass = env->GetObjectClass(g_mainActivity);
        if (activityClass) {
            g_playBGMMethodId = env->GetMethodID(activityClass, "playBGM", "(Ljava/lang/String;)V");
            g_stopBGMMethodId = env->GetMethodID(activityClass, "stopBGM", "()V");
            g_playSEMethodId = env->GetMethodID(activityClass, "playSE", "(Ljava/lang/String;)V");

            if (!g_playBGMMethodId || !g_stopBGMMethodId || !g_playSEMethodId) {
                LOGE("Failed to cache audio method IDs");
            } else {
                LOGD("Audio method IDs cached successfully");
            }

            env->DeleteLocalRef(activityClass);
        }
    } else {
        LOGE("Failed to create global reference to MainActivity");
    }
}

void jni_audio_call_play_bgm(const char* path) {
    if (!g_javaVM || !g_mainActivity || !g_playBGMMethodId) {
        LOGW("Audio JNI not ready: vm=%p, activity=%p, methodId=%p",
             g_javaVM, g_mainActivity, g_playBGMMethodId);
        return;
    }

    JNIEnv* env = nullptr;
    bool needsDetach = false;

    // JNIEnv を取得（スレッドアタッチが必要な場合がある）
    int attachStatus = g_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (attachStatus == JNI_EDETACHED) {
        g_javaVM->AttachCurrentThread(&env, nullptr);
        needsDetach = true;
    }

    if (!env) {
        LOGE("Failed to get JNI environment for playBGM");
        return;
    }

    // Java String を作成
    jstring javaPath = env->NewStringUTF(path ? path : "");

    // playBGM(path) を呼び出し
    env->CallVoidMethod(g_mainActivity, g_playBGMMethodId, javaPath);

    // 例外チェック
    if (env->ExceptionCheck()) {
        LOGE("JNI exception in playBGM");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    // クリーンアップ
    env->DeleteLocalRef(javaPath);

    if (needsDetach) {
        g_javaVM->DetachCurrentThread();
    }

    LOGD("playBGM called: %s", path ? path : "(null)");
}

void jni_audio_call_stop_bgm() {
    if (!g_javaVM || !g_mainActivity || !g_stopBGMMethodId) {
        LOGW("Audio JNI not ready for stopBGM");
        return;
    }

    JNIEnv* env = nullptr;
    bool needsDetach = false;

    int attachStatus = g_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (attachStatus == JNI_EDETACHED) {
        g_javaVM->AttachCurrentThread(&env, nullptr);
        needsDetach = true;
    }

    if (!env) {
        LOGE("Failed to get JNI environment for stopBGM");
        return;
    }

    env->CallVoidMethod(g_mainActivity, g_stopBGMMethodId);

    if (env->ExceptionCheck()) {
        LOGE("JNI exception in stopBGM");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    if (needsDetach) {
        g_javaVM->DetachCurrentThread();
    }

    LOGD("stopBGM called");
}

void jni_audio_call_play_se(const char* path) {
    if (!g_javaVM || !g_mainActivity || !g_playSEMethodId) {
        LOGW("Audio JNI not ready for playSE");
        return;
    }

    JNIEnv* env = nullptr;
    bool needsDetach = false;

    int attachStatus = g_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (attachStatus == JNI_EDETACHED) {
        g_javaVM->AttachCurrentThread(&env, nullptr);
        needsDetach = true;
    }

    if (!env) {
        LOGE("Failed to get JNI environment for playSE");
        return;
    }

    jstring javaPath = env->NewStringUTF(path ? path : "");
    env->CallVoidMethod(g_mainActivity, g_playSEMethodId, javaPath);

    if (env->ExceptionCheck()) {
        LOGE("JNI exception in playSE");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    env->DeleteLocalRef(javaPath);

    if (needsDetach) {
        g_javaVM->DetachCurrentThread();
    }

    LOGD("playSE called: %s", path ? path : "(null)");
}

}  // extern "C"
