#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "engine/renderer.h"

#define LOG_TAG "JNI_Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static Renderer* g_renderer = nullptr;
AAssetManager* g_assetManager = nullptr;

extern "C" {
    void jni_audio_set_asset_manager(AAssetManager* mgr);
    void jni_audio_set_java_vm(JavaVM* vm);
    void jni_audio_set_main_activity(jobject activity);
}

// Initialize engine and return handle to Java
extern "C" JNIEXPORT jlong JNICALL
Java_com_example_oblivion_GameRenderer_nativeInitEngine(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj) {
    LOGI("=== nativeInitEngine called ===");

    if (g_renderer != nullptr) {
        LOGD("Renderer already initialized, returning existing handle: %p", g_renderer);
        return reinterpret_cast<jlong>(g_renderer);
    }

    LOGI("Creating new Renderer instance...");
    try {
        LOGI("STEP 1: Allocating Renderer object...");
        g_renderer = new Renderer();
        LOGI("STEP 1: SUCCESS - Renderer object allocated");

        LOGI("STEP 2: Calling Renderer::init(1920, 1080)...");
        bool initResult = g_renderer->init(1920, 1080);  // Default size, will be updated by onSurfaceChanged

        LOGI("STEP 2: init() returned %d", initResult ? 1 : 0);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "===== INIT RESULT: %s =====",
                            initResult ? "SUCCESS" : "FAILED");

        if (!initResult) {
            LOGE("CRITICAL: Renderer::init() returned false - initialization failed");
            LOGE("Deleting Renderer instance...");
            delete g_renderer;
            g_renderer = nullptr;
            LOGE("Renderer deleted, returning 0 to Java");
            return 0;
        }

        LOGI("SUCCESS: Renderer initialized successfully");
        jlong handle = reinterpret_cast<jlong>(g_renderer);
        LOGI("Returning handle to Java: %p", g_renderer);
        return handle;
    } catch (const std::exception& e) {
        LOGE("EXCEPTION in nativeInitEngine: %s", e.what());
        LOGE("Exception type details: checking std::exception");
        if (g_renderer) {
            delete g_renderer;
            g_renderer = nullptr;
        }
        return 0;
    } catch (...) {
        LOGE("UNKNOWN EXCEPTION in nativeInitEngine");
        if (g_renderer) {
            delete g_renderer;
            g_renderer = nullptr;
        }
        return 0;
    }
}

// Initialize audio bridge with asset manager & JavaVM (Phase 8+)
extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeInitAudioBridge(
        JNIEnv* env,
        jobject mainActivityObj,
        jobject assetManager) {
    LOGI("nativeInitAudioBridge called");

    if (!assetManager) {
        LOGE("AssetManager is null");
        return;
    }

    // AAssetManager を取得してグローバル変数に設定（TextRenderer や Audio で使用）
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        LOGE("Failed to get AAssetManager from Java");
        return;
    }
    g_assetManager = mgr;
    LOGI("g_assetManager set successfully: %p", g_assetManager);

#ifdef AUDIO_SYSTEM_ENABLED
    jni_audio_set_asset_manager(mgr);
    LOGD("AAssetManager set for audio system");

    // JavaVM を取得して設定
    JavaVM* vm = nullptr;
    if (env->GetJavaVM(&vm) != JNI_OK) {
        LOGE("Failed to get JavaVM");
        return;
    }
    jni_audio_set_java_vm(vm);
    LOGD("JavaVM set");

    // MainActivity インスタンスを設定
    if (mainActivityObj) {
        jni_audio_set_main_activity(mainActivityObj);
        LOGD("MainActivity reference set");
    } else {
        LOGE("MainActivity object is null");
        return;
    }
#else
    LOGD("Audio system disabled, skipping audio bridge setup");
#endif

    LOGI("Asset manager bridge initialized successfully");
}

// Set viewport with handle parameter
extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeSetViewport(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jlong handle,
        jint width,
        jint height) {
    LOGD("nativeSetViewport called: %d x %d", width, height);

    Renderer* renderer = reinterpret_cast<Renderer*>(handle);
    if (renderer) {
        renderer->resize(width, height);
    }
}

// Render frame with handle parameter
extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeRenderFrame(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jlong handle) {
    Renderer* renderer = reinterpret_cast<Renderer*>(handle);
    if (renderer) {
        renderer->render(0.0167f);  // 60 FPS default (1/60 sec)
    } else {
        LOGD("WARNING: nativeRenderFrame called with null renderer handle");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeCleanup(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj) {
    LOGI("nativeCleanup called");

    if (g_renderer) {
        g_renderer->cleanup();
        delete g_renderer;
        g_renderer = nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeSetLanguage(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jint language) {
    LOGD("nativeSetLanguage called: %d", language);

    if (g_renderer) {
        auto locMgr = g_renderer->getLocalizationManager();
        if (locMgr) {
            locMgr->setLanguage(static_cast<Language>(language));
        }
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_oblivion_GameRenderer_nativeGetLanguage(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj) {
    if (g_renderer) {
        auto locMgr = g_renderer->getLocalizationManager();
        if (locMgr) {
            return static_cast<jint>(locMgr->getLanguage());
        }
    }
    return 0;  // ENGLISH
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_oblivion_GameRenderer_nativeGetString(
        JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jstring key) {
    if (!g_renderer) {
        return env->NewStringUTF("");
    }

    auto locMgr = g_renderer->getLocalizationManager();
    if (!locMgr) {
        return env->NewStringUTF("");
    }

    const char* keyStr = env->GetStringUTFChars(key, nullptr);
    std::string result = locMgr->getString(keyStr);
    env->ReleaseStringUTFChars(key, keyStr);

    return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeOnTouchEvent(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jlong handle,
        jfloat dx,
        jfloat dy) {
    Renderer* renderer = reinterpret_cast<Renderer*>(handle);
    if (renderer && renderer->getTitleScreen()) {
        renderer->getTitleScreen()->onTouchEvent(dx, dy);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeOnKeyPress(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jint key) {
    if (g_renderer && g_renderer->getTitleScreen()) {
        g_renderer->getTitleScreen()->onKeyPress(key);
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_oblivion_GameRenderer_nativeTitleScreenActive(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj) {
    if (g_renderer) {
        return g_renderer->isTitleScreenActive() ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeSetTargetFPS(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj,
        jint fps) {
    LOGD("nativeSetTargetFPS called: %d", fps);

    if (g_renderer) {
        g_renderer->setTargetFPS(static_cast<int>(fps));
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_oblivion_GameRenderer_nativeGetTargetFPS(
        [[maybe_unused]] JNIEnv* env,
        [[maybe_unused]] jobject obj) {
    if (g_renderer) {
        return static_cast<jint>(g_renderer->getTargetFPS());
    }
    return 60;  // Default
}
