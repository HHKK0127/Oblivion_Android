#include "com_example_oblivion_OblivionEngine.h"
#include "../engine/Engine.h"
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <mutex>
#include <unordered_map>

#define LOG_TAG "OblivionJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Thread-safe engine instance management
static std::mutex engineMutex;
static std::unordered_map<jlong, std::unique_ptr<oblivion::Engine>> engines;
static jlong nextEngineHandle = 1;

// Legacy: Static engine instance (deprecated)
std::unique_ptr<oblivion::Engine> OblivionEngineJNI::sEngine = nullptr;

// Helper: Get engine from handle
static oblivion::Engine* getEngineFromHandle(jlong handle) {
    if (handle == 0) return nullptr;
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = engines.find(handle);
    return (it != engines.end()) ? it->second.get() : nullptr;
}

// JNI_OnLoad
jint JNI_OnLoad(JavaVM* /* vm */, void* /* reserved */) {
    LOGI("JNI_OnLoad called");
    return JNI_VERSION_1_6;
}

// JNI Methods
extern "C" {

JNIEXPORT jlong JNICALL Java_com_example_oblivion_OblivionEngine_nativeInitialize(
    JNIEnv* env,
    jobject /* obj */,
    jobject surface,
    jboolean enableValidation) {

    LOGI("nativeInitialize called (enableValidation=%d)", enableValidation);

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        LOGE("Failed to get ANativeWindow from Surface");
        return 0;
    }

    auto engine = std::make_unique<oblivion::Engine>();

    oblivion::InitParams params;
    params.androidApp = nullptr;
    params.nativeWindow = window;

    LOGI("Calling engine->init()...");
    if (!engine->init(params)) {
        LOGE("Failed to initialize engine");
        ANativeWindow_release(window);
        return 0;
    }

    // Store engine in thread-safe map
    {
        std::lock_guard<std::mutex> lock(engineMutex);
        jlong handle = nextEngineHandle++;
        engines[handle] = std::move(engine);
        LOGI("Engine initialized successfully with handle %ld", handle);
        return handle;
    }
}

JNIEXPORT jlong JNICALL Java_com_example_oblivion_OblivionEngine_nativeCreate(
    JNIEnv* /* env */,
    jobject /* obj */,
    jobject androidApp) {

    LOGI("nativeCreate called");

    if (!OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine = std::make_unique<oblivion::Engine>();

        oblivion::InitParams params;
        params.androidApp = androidApp;
        params.nativeWindow = nullptr;

        if (!OblivionEngineJNI::sEngine->init(params)) {
            LOGE("Failed to initialize engine");
            OblivionEngineJNI::sEngine.reset();
            return 0;
        }
    }

    return reinterpret_cast<jlong>(OblivionEngineJNI::sEngine.get());
}

JNIEXPORT jlong JNICALL Java_com_example_oblivion_OblivionEngine_nativeGetHandle(
    JNIEnv* /* env */,
    jobject /* obj */) {

    if (OblivionEngineJNI::sEngine) {
        return reinterpret_cast<jlong>(OblivionEngineJNI::sEngine.get());
    }
    return 0;
}

JNIEXPORT jboolean JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceCreated(
    JNIEnv* env,
    jobject /* obj */,
    jlong /* handle */,
    jobject surface) {

    LOGI("nativeOnSurfaceCreated called");

    if (!OblivionEngineJNI::sEngine) {
        LOGE("Engine not created");
        return JNI_FALSE;
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return JNI_FALSE;
    }

    if (!OblivionEngineJNI::sEngine->onSurfaceCreated(window)) {
        LOGE("Failed to create surface");
        ANativeWindow_release(window);
        return JNI_FALSE;
    }

    // Start game loop thread
    OblivionEngineJNI::sEngine->startLoop();

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceChanged(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */,
    jint width,
    jint height) {

    LOGI("nativeOnSurfaceChanged called: %dx%d", width, height);

    if (!OblivionEngineJNI::sEngine) {
        return JNI_FALSE;
    }

    if (!OblivionEngineJNI::sEngine->onSurfaceChanged(width, height)) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceDestroyed(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */) {

    LOGI("nativeOnSurfaceDestroyed called");

    if (OblivionEngineJNI::sEngine) {
        // Stop game loop first
        OblivionEngineJNI::sEngine->stopLoop();
        OblivionEngineJNI::sEngine->onSurfaceDestroyed();
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativePause(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */) {

    LOGI("nativePause called - Game loop paused");

    if (OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine->pause();
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeResume(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */) {

    LOGI("nativeResume called - Game loop resumed");

    if (OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine->resume();
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnTouchEvent(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong /* handle */,
    jfloat x,
    jfloat y,
    jint action,
    jint pointerId) {

    if (OblivionEngineJNI::sEngine) {
        OblivionEngineJNI::sEngine->queueTouchEvent(pointerId, x, y, action);
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeDestroy(
    JNIEnv* /* env */,
    jobject /* obj */,
    jlong handle) {

    LOGI("nativeDestroy called (handle=%ld)", handle);

    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = engines.find(handle);
    if (it != engines.end()) {
        LOGI("Destroying engine with handle %ld", handle);
        it->second->shutdown();
        engines.erase(it);
    } else {
        LOGE("Engine not found: handle=%ld", handle);
    }
}

// UI Control Methods

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleCharacterSheet(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeToggleCharacterSheet called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->toggleCharacterSheet();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleSpellbook(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeToggleSpellbook called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->toggleSpellbook();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeToggleQuestLog(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeToggleQuestLog called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->toggleQuestLog();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeCloseDialogue(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeCloseDialogue called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->closeDialogue();
        }
    }
}

JNIEXPORT void JNICALL Java_com_example_oblivion_OblivionEngine_nativeCloseShop(
    JNIEnv* /* env */,
    jobject /* obj */) {

    LOGI("nativeCloseShop called");

    if (OblivionEngineJNI::sEngine) {
        auto uiManager = OblivionEngineJNI::sEngine->getUIManager();
        if (uiManager) {
            uiManager->closeShop();
        }
    }
}

} // extern "C"
