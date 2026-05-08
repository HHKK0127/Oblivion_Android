#include <jni.h>
#include <android/log.h>
#include "engine/renderer.h"

#define LOG_TAG "JNI_Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static Renderer* g_renderer = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeInit(
        JNIEnv* env,
        jobject obj,
        jint width,
        jint height) {
    LOGI("nativeInit called: %d x %d", width, height);

    if (g_renderer == nullptr) {
        g_renderer = new Renderer();
        if (!g_renderer->init(width, height)) {
            LOGE("Failed to initialize Renderer");
            delete g_renderer;
            g_renderer = nullptr;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeRender(
        JNIEnv* env,
        jobject obj,
        jfloat deltaTime) {
    if (g_renderer) {
        g_renderer->render(deltaTime);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeCleanup(
        JNIEnv* env,
        jobject obj) {
    LOGI("nativeCleanup called");

    if (g_renderer) {
        g_renderer->cleanup();
        delete g_renderer;
        g_renderer = nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeSetLanguage(
        JNIEnv* env,
        jobject obj,
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
        JNIEnv* env,
        jobject obj) {
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
        jobject obj,
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
        JNIEnv* env,
        jobject obj,
        jfloat x,
        jfloat y) {
    if (g_renderer && g_renderer->getTitleScreen()) {
        g_renderer->getTitleScreen()->onTouchEvent(x, y);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeOnKeyPress(
        JNIEnv* env,
        jobject obj,
        jint key) {
    if (g_renderer && g_renderer->getTitleScreen()) {
        g_renderer->getTitleScreen()->onKeyPress(key);
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_oblivion_GameRenderer_nativeTitleScreenActive(
        JNIEnv* env,
        jobject obj) {
    if (g_renderer) {
        return g_renderer->isTitleScreenActive() ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_oblivion_GameRenderer_nativeSetTargetFPS(
        JNIEnv* env,
        jobject obj,
        jint fps) {
    LOGD("nativeSetTargetFPS called: %d", fps);

    if (g_renderer) {
        g_renderer->setTargetFPS(static_cast<int>(fps));
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_oblivion_GameRenderer_nativeGetTargetFPS(
        JNIEnv* env,
        jobject obj) {
    if (g_renderer) {
        return static_cast<jint>(g_renderer->getTargetFPS());
    }
    return 60;  // Default
}
