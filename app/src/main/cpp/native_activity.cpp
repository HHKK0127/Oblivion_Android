#include <jni.h>
#include <errno.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "engine/renderer.h"
#include "jni_audio_bridge.h"

#define LOG_TAG "NativeActivity"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// EGL context and surface management
struct AppState {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    EGLConfig config;

    ANativeWindow* window;
    Renderer* renderer;

    int width;
    int height;

    std::thread render_thread;
    std::mutex mutex;
    std::condition_variable cond_var;

    bool should_render;
    bool render_ready;
    bool window_changed;

    AppState() : display(EGL_NO_DISPLAY), context(EGL_NO_CONTEXT),
                 surface(EGL_NO_SURFACE), config(nullptr),
                 window(nullptr), renderer(nullptr),
                 width(0), height(0),
                 should_render(false), render_ready(false),
                 window_changed(false) {}
};

// Initialize EGL
static bool initEGL(AppState* state) {
    LOGI("Initializing EGL");

    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (state->display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(state->display, &majorVersion, &minorVersion)) {
        LOGE("eglInitialize failed");
        return false;
    }

    LOGI("EGL initialized: %d.%d", majorVersion, minorVersion);

    // Choose EGL config
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    EGLint numConfigs;
    if (!eglChooseConfig(state->display, configAttribs, &state->config, 1, &numConfigs)) {
        LOGE("eglChooseConfig failed");
        return false;
    }

    LOGI("EGL config chosen");

    // Create EGL context
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    state->context = eglCreateContext(state->display, state->config, nullptr, contextAttribs);
    if (state->context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }

    LOGI("EGL context created");
    return true;
}

// Create EGL surface
static bool createSurface(AppState* state) {
    if (!state->window) {
        LOGE("createSurface: window is null");
        return false;
    }

    LOGI("Creating EGL surface");

    state->surface = eglCreateWindowSurface(state->display, state->config, state->window, nullptr);
    if (state->surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed: 0x%x", eglGetError());
        return false;
    }

    if (!eglMakeCurrent(state->display, state->surface, state->surface, state->context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError());
        return false;
    }

    // Set swap interval (vsync)
    eglSwapInterval(state->display, 1);

    // Get surface dimensions
    eglQuerySurface(state->display, state->surface, EGL_WIDTH, &state->width);
    eglQuerySurface(state->display, state->surface, EGL_HEIGHT, &state->height);

    LOGI("Surface created: %dx%d", state->width, state->height);

    // Initialize Renderer
    if (!state->renderer) {
        state->renderer = new Renderer();
        if (!state->renderer->init(state->width, state->height)) {
            LOGE("Failed to initialize Renderer");
            return false;
        }
        LOGI("Renderer initialized");
    }

    return true;
}

// Rendering thread
static void renderingThread(AppState* state) {
    LOGI("Rendering thread started");

    while (true) {
        std::unique_lock<std::mutex> lock(state->mutex);

        // Wait for signal to render
        state->cond_var.wait(lock, [state] { return state->should_render || state->window_changed; });

        if (!state->should_render && !state->window_changed) {
            break;  // Exit thread
        }

        if (state->window_changed) {
            state->window_changed = false;
            if (state->window) {
                if (state->surface != EGL_NO_SURFACE) {
                    eglDestroySurface(state->display, state->surface);
                    state->surface = EGL_NO_SURFACE;
                }
                if (createSurface(state)) {
                    state->render_ready = true;
                } else {
                    state->render_ready = false;
                    LOGE("Failed to create surface in rendering thread");
                }
            } else {
                state->render_ready = false;
            }
            continue;
        }

        if (!state->render_ready || !state->renderer) {
            continue;
        }

        lock.unlock();

        // Render frame
        if (state->renderer) {
            state->renderer->render(0.0167f);  // 60 FPS
            eglSwapBuffers(state->display, state->surface);
        }
    }

    LOGI("Rendering thread exiting");
}

// ANativeActivity callbacks
static void onStart(ANativeActivity* activity) {
    LOGI("onStart");
}

static void onResume(ANativeActivity* activity) {
    LOGI("onResume");
    AppState* state = (AppState*)activity->instance;
    if (state) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->should_render = true;
        state->cond_var.notify_one();
    }
}

static void onPause(ANativeActivity* activity) {
    LOGI("onPause");
    AppState* state = (AppState*)activity->instance;
    if (state) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->should_render = false;
    }
}

static void onStop(ANativeActivity* activity) {
    LOGI("onStop");
}

static void onDestroy(ANativeActivity* activity) {
    LOGI("onDestroy");
    AppState* state = (AppState*)activity->instance;
    if (state) {
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->should_render = false;
        }
        state->cond_var.notify_one();

        // Wait for render thread to finish
        if (state->render_thread.joinable()) {
            state->render_thread.join();
        }

        // Cleanup EGL
        if (state->display != EGL_NO_DISPLAY) {
            eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (state->context != EGL_NO_CONTEXT) {
                eglDestroyContext(state->display, state->context);
            }
            if (state->surface != EGL_NO_SURFACE) {
                eglDestroySurface(state->display, state->surface);
            }
            eglTerminate(state->display);
        }

        // Cleanup renderer
        if (state->renderer) {
            state->renderer->cleanup();
            delete state->renderer;
        }

        delete state;
        activity->instance = nullptr;
    }
}

static void onWindowFocusChanged(ANativeActivity* activity, int hasFocus) {
    LOGI("onWindowFocusChanged: %d", hasFocus);
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    LOGI("onNativeWindowCreated");
    AppState* state = (AppState*)activity->instance;
    if (state) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->window = window;
        state->window_changed = true;
        state->cond_var.notify_one();
    }
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    LOGI("onNativeWindowDestroyed");
    AppState* state = (AppState*)activity->instance;
    if (state) {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->window = nullptr;
        state->render_ready = false;
    }
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
    LOGI("onInputQueueCreated");
    // Input events will be routed to app->onInputEvent callback
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
    LOGI("onInputQueueDestroyed");
}

static void onConfigurationChanged(ANativeActivity* activity) {
    LOGI("onConfigurationChanged");
}

static void onLowMemory(ANativeActivity* activity) {
    LOGI("onLowMemory");
}

// ============================================================================
// JNI Library Initialization
// ============================================================================
// Called when the native library is loaded
extern "C" {

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad called");

    // NOTE: JNI audio bridge initialization disabled - requires Java MainActivity class
    // which is not available in pure NativeActivity context. Audio will be deferred.
    // jni_audio_bridge_init(vm);

    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnUnload called");

    // NOTE: JNI audio bridge cleanup disabled - audio deferred
    // jni_audio_bridge_cleanup();
}

}  // extern "C"

// Main entry point for ANativeActivity
void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    LOGI("=== ANativeActivity_onCreate ===");

    // NOTE: JNI audio bridge initialization disabled - requires Java MainActivity class
    // which is not available in pure NativeActivity context. Audio will be deferred.
    // jni_audio_bridge_init(activity->vm);
    // LOGI("JNI audio bridge initialized from ANativeActivity");

    AppState* state = new AppState();
    activity->instance = state;

    // Set callbacks
    activity->callbacks->onStart = onStart;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onPause = onPause;
    activity->callbacks->onStop = onStop;
    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onConfigurationChanged = onConfigurationChanged;
    activity->callbacks->onLowMemory = onLowMemory;

    // Initialize EGL
    if (!initEGL(state)) {
        LOGE("EGL initialization failed");
        delete state;
        return;
    }

    // Start rendering thread
    state->should_render = false;
    state->render_thread = std::thread(renderingThread, state);

    LOGI("ANativeActivity_onCreate completed");
}

// ============================================================================
// JNI Bridge for Audio System
// ============================================================================
// Called from Java/Kotlin MainActivity

extern "C" {

/**
 * Play BGM from C++
 * This is called from audio_manager.cpp via playBGMViaJava()
 * It delegates to the JNI audio bridge
 */
JNIEXPORT void JNICALL Java_com_example_oblivion_MainActivity_playBGM_JNI
        (JNIEnv *env, jclass clazz, jstring filename) {
    const char *filenameStr = env->GetStringUTFChars(filename, nullptr);
    LOGI("JNI: playBGM_JNI called with file: %s", filenameStr);

    // Forward to the JNI audio bridge which handles the actual Java call
    if (filenameStr) {
        jni_audio_play_bgm(filenameStr);
    }

    env->ReleaseStringUTFChars(filename, filenameStr);
}

/**
 * Play SE from C++
 * This is called from audio_manager.cpp via playSEViaJava()
 * It delegates to the JNI audio bridge
 */
JNIEXPORT void JNICALL Java_com_example_oblivion_MainActivity_playSE_JNI
        (JNIEnv *env, jclass clazz, jstring filename) {
    const char *filenameStr = env->GetStringUTFChars(filename, nullptr);
    LOGI("JNI: playSE_JNI called with file: %s", filenameStr);

    // Forward to the JNI audio bridge which handles the actual Java call
    if (filenameStr) {
        jni_audio_play_se(filenameStr);
    }

    env->ReleaseStringUTFChars(filename, filenameStr);
}

/**
 * Stop BGM from C++
 * This is called from audio_manager.cpp via stopBGMViaJava()
 * It delegates to the JNI audio bridge
 */
JNIEXPORT void JNICALL Java_com_example_oblivion_MainActivity_stopBGM_JNI
        (JNIEnv *env, jclass clazz) {
    LOGI("JNI: stopBGM_JNI called");

    // Forward to the JNI audio bridge which handles the actual Java call
    jni_audio_stop_bgm();
}

}  // extern "C"
