# Oblivion Android - JNI ブリッジ詳細設計書

**作成日**: 2026-06-06  
**バージョン**: 1.0  
**ステータス**: 詳細設計フェーズ

---

## 目次

1. [システム概要](#システム概要)
2. [システムアーキテクチャ](#システムアーキテクチャ)
3. [Java/Kotlin 側の設計](#javalauncher-kotlin-側の設計)
4. [C++ 側の JNI ブリッジ](#c-側の-jni-ブリッジ)
5. [通信プロトコル](#通信プロトコル)
6. [ライフサイクル管理](#ライフサイクル管理)
7. [エラーハンドリング戦略](#エラーハンドリング戦略)
8. [メモリ管理](#メモリ管理)
9. [実装フロー](#実装フロー)
10. [テスト方針](#テスト方針)

---

## システム概要

本設計書は、The Elder Scrolls: Oblivion をAndroidプラットフォームに移植するための**JNIブリッジアーキテクチャ**を定義します。

### 主要な目標

- **Kotlinネイティブ統合**: Android LifecycleとOblivion Engine の同期
- **Vulkanグラフィックス**: JNI経由での安全なVulkan制御
- **スレッド安全性**: マルチスレッド環境での堅牢な通信
- **リソース管理**: メモリリークおよびネイティブリソースの厳密な管理
- **エラー回復**: 例外ハンドリングと復旧メカニズム

### プロジェクト構成

```
oblivion-android/
├── app/src/main/
│   ├── java/com/example/oblivion/
│   │   ├── OblivionEngine.kt           # JNI ブリッジ (シングルトン)
│   │   ├── GameActivity.kt              # メイン Activity
│   │   ├── GameSurfaceView.kt           # Surface 管理
│   │   ├── InputHandler.kt              # 入力処理
│   │   └── PlatformServices.kt          # Android 機能抽象
│   └── cpp/
│       ├── jni/
│       │   ├── com_example_oblivion_OblivionEngine.h
│       │   ├── com_example_oblivion_OblivionEngine.cpp
│       │   └── jni_utils.h
│       ├── engine/
│       │   ├── Engine.h
│       │   └── Engine.cpp
│       └── CMakeLists.txt
└── docs/
    ├── JNI_BRIDGE_DESIGN.md            # 本書
    └── ARCHITECTURE.md                 # アーキテクチャ詳細
```

---

## システムアーキテクチャ

### 全体構造図

```
┌─────────────────────────────────────────────────────────────┐
│                    Android Framework                        │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          GameActivity (Main Thread)                 │   │
│  │  • onCreate, onResume, onPause, onDestroy          │   │
│  │  • Surface/SurfaceHolder 管理                      │   │
│  │  • ネイティブ初期化・終了制御                       │   │
│  └──────────────┬──────────────────────────────────────┘   │
│                 │                                            │
│  ┌──────────────▼──────────────────────────────────────┐   │
│  │       OblivionEngine (Singleton Wrapper)            │   │
│  │  • nativeInit(AssetManager, Context)                │   │
│  │  • nativeOnSurfaceCreated(Surface)                  │   │
│  │  • nativeOnSurfaceChanged(width, height)            │   │
│  │  • nativePause()                                    │   │
│  │  • nativeResume()                                   │   │
│  │  • nativeDestroy()                                  │   │
│  │  • nativeOnTouchEvent(x, y, action)                 │   │
│  │  • メモリ管理・同期化                               │   │
│  └──────────────┬──────────────────────────────────────┘   │
│                 │ JNI Calls                                  │
└─────────────────┼──────────────────────────────────────────┘
                  │
┌─────────────────▼──────────────────────────────────────────┐
│                  JNI Bridge Layer                            │
│  ┌────────────────────────────────────────────────────┐    │
│  │  com_example_oblivion_OblivionEngine_JNIEXPORT     │    │
│  │  • JNI Function Stubs                              │    │
│  │  • Kotlin ↔ C++ Data Conversion                    │    │
│  │  • Exception Handling                              │    │
│  │  • Thread Safety (Mutex/Lock)                      │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────┬──────────────────────────────────────────┘
                  │
┌─────────────────▼──────────────────────────────────────────┐
│                 C++ Engine Layer                             │
│  ┌────────────────────────────────────────────────────┐    │
│  │  class Engine                                       │    │
│  │  • init()  - Vulkan + Asset initialization         │    │
│  │  • update() - Game logic update                    │    │
│  │  • render() - Vulkan render commands               │    │
│  │  • shutdown() - Cleanup                            │    │
│  │  • onSurfaceChanged() - Framebuffer handling       │    │
│  └────────────────────────────────────────────────────┘    │
│  ┌────────────────────────────────────────────────────┐    │
│  │  Subsystems                                         │    │
│  │  • VulkanRenderer    (Graphics)                    │    │
│  │  • InputSystem       (Touch events)                │    │
│  │  • GameWorld         (Game logic)                  │    │
│  │  • AudioSystem       (Sound)                       │    │
│  │  • FileSystem        (Asset loading)               │    │
│  └────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

### スレッド構造

```
Main Thread (Android UI Thread)
  ├── Activity Lifecycle
  │   └── JNI Calls (OblivionEngine)
  │       └── Engine::onXxx() [Quick return]
  │
Render Thread (Vulkan Rendering)
  └── Engine::renderLoop()
      ├── Vulkan command recording
      └── Queue submission

Input Thread (Optional Event Dispatcher)
  └── TouchEvent → InputSystem
```

---

## Java/Kotlin 側の設計

### 1.1 OblivionEngine クラス（シングルトン）

**責務**:
- ネイティブメソッド定義
- メモリライフサイクル管理
- Java ↔ C++ 例外ハンドリング
- スレッドセーフな呼び出し

**実装ファイル**: `app/src/main/java/com/example/oblivion/OblivionEngine.kt`

```kotlin
package com.example.oblivion

import android.content.Context
import android.content.res.AssetManager
import android.view.Surface
import android.util.Log
import java.util.concurrent.locks.ReentrantReadWriteLock

/**
 * OblivionEngine シングルトン
 * 
 * C++ Engine への JNI ブリッジ。
 * スレッドセーフなネイティブメソッド呼び出しを提供する。
 */
object OblivionEngine {
    private const val TAG = "OblivionEngine"
    private const val NATIVE_LIB_NAME = "oblivion-engine"
    
    // ネイティブハンドル (不透明なポインタ)
    private var nativeHandle: Long = 0
    private val lock = ReentrantReadWriteLock()
    
    // 初期化状態フラグ
    @Volatile
    private var initialized = false
    
    /**
     * ライブラリの読み込みと初期化
     */
    init {
        try {
            System.loadLibrary(NATIVE_LIB_NAME)
            Log.d(TAG, "Native library loaded successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native library: ${e.message}")
        }
    }
    
    /**
     * エンジン初期化
     * 
     * @param context Android Context
     * @param assetManager AssetManager for asset loading
     * @return true if successful
     * @throws OblivionException 初期化失敗時
     */
    @Throws(OblivionException::class)
    fun initialize(context: Context, assetManager: AssetManager): Boolean {
        lock.writeLock().lock()
        try {
            if (initialized) {
                Log.w(TAG, "Engine already initialized")
                return true
            }
            
            Log.d(TAG, "Initializing engine...")
            val cacheDir = context.cacheDir.absolutePath
            val assetDir = context.filesDir.absolutePath
            
            val result = nativeInit(assetManager, cacheDir, assetDir)
            
            if (result != JNI_OK) {
                val errorMsg = "Native init failed with code: $result"
                Log.e(TAG, errorMsg)
                throw OblivionException(errorMsg, result)
            }
            
            initialized = true
            nativeHandle = nativeGetHandle()
            Log.d(TAG, "Engine initialized successfully. Handle: $nativeHandle")
            return true
            
        } catch (e: Exception) {
            Log.e(TAG, "Initialization error: ${e.message}", e)
            throw if (e is OblivionException) e else OblivionException("Init failed", e)
        } finally {
            lock.writeLock().unlock()
        }
    }
    
    /**
     * Surface 生成時の処理
     */
    @Throws(OblivionException::class)
    fun onSurfaceCreated(surface: Surface) {
        lock.readLock().lock()
        try {
            if (!initialized) throw OblivionException("Engine not initialized")
            
            Log.d(TAG, "onSurfaceCreated")
            val result = nativeOnSurfaceCreated(surface)
            
            if (result != JNI_OK) {
                throw OblivionException("onSurfaceCreated failed: $result", result)
            }
        } finally {
            lock.readLock().unlock()
        }
    }
    
    /**
     * Surface 変更時の処理
     */
    @Throws(OblivionException::class)
    fun onSurfaceChanged(width: Int, height: Int) {
        lock.readLock().lock()
        try {
            if (!initialized) throw OblivionException("Engine not initialized")
            
            Log.d(TAG, "onSurfaceChanged: $width x $height")
            val result = nativeOnSurfaceChanged(width, height)
            
            if (result != JNI_OK) {
                throw OblivionException("onSurfaceChanged failed: $result", result)
            }
        } finally {
            lock.readLock().unlock()
        }
    }
    
    /**
     * Surface 破棄時の処理
     */
    @Throws(OblivionException::class)
    fun onSurfaceDestroyed() {
        lock.readLock().lock()
        try {
            if (!initialized) return
            
            Log.d(TAG, "onSurfaceDestroyed")
            nativeOnSurfaceDestroyed()
        } finally {
            lock.readLock().unlock()
        }
    }
    
    /**
     * ゲーム一時停止
     */
    @Throws(OblivionException::class)
    fun pause() {
        lock.readLock().lock()
        try {
            if (!initialized) return
            
            Log.d(TAG, "Pausing engine")
            nativePause()
        } finally {
            lock.readLock().unlock()
        }
    }
    
    /**
     * ゲーム再開
     */
    @Throws(OblivionException::class)
    fun resume() {
        lock.readLock().lock()
        try {
            if (!initialized) return
            
            Log.d(TAG, "Resuming engine")
            nativeResume()
        } finally {
            lock.readLock().unlock()
        }
    }
    
    /**
     * タッチイベント処理
     */
    @Throws(OblivionException::class)
    fun onTouchEvent(x: Float, y: Float, action: Int, pointerId: Int = 0) {
        lock.readLock().lock()
        try {
            if (!initialized) return
            
            nativeOnTouchEvent(x, y, action, pointerId)
        } finally {
            lock.readLock().unlock()
        }
    }
    
    /**
     * エンジン破棄・クリーンアップ
     */
    @Throws(OblivionException::class)
    fun destroy() {
        lock.writeLock().lock()
        try {
            if (!initialized) return
            
            Log.d(TAG, "Destroying engine")
            nativeDestroy()
            initialized = false
            nativeHandle = 0
        } catch (e: Exception) {
            Log.e(TAG, "Error during destroy: ${e.message}", e)
            throw if (e is OblivionException) e else OblivionException("Destroy failed", e)
        } finally {
            lock.writeLock().unlock()
        }
    }
    
    /**
     * 初期化状態の確認
     */
    fun isInitialized(): Boolean {
        lock.readLock().lock()
        try {
            return initialized
        } finally {
            lock.readLock().unlock()
        }
    }
    
    // ====== JNI ネイティブメソッド定義 ======
    
    private external fun nativeInit(
        assetManager: AssetManager,
        cacheDir: String,
        assetDir: String
    ): Int
    
    private external fun nativeGetHandle(): Long
    
    private external fun nativeOnSurfaceCreated(surface: Surface): Int
    
    private external fun nativeOnSurfaceChanged(width: Int, height: Int): Int
    
    private external fun nativeOnSurfaceDestroyed()
    
    private external fun nativePause()
    
    private external fun nativeResume()
    
    private external fun nativeOnTouchEvent(x: Float, y: Float, action: Int, pointerId: Int)
    
    private external fun nativeDestroy()
    
    companion object {
        private const val JNI_OK = 0
    }
}

/**
 * Oblivion Exception
 */
class OblivionException(
    message: String,
    val errorCode: Int = -1,
    cause: Throwable? = null
) : Exception(message, cause)
```

### 1.2 GameActivity（メインスレッド）

**責務**:
- Android ライフサイクル管理
- Surface/SurfaceHolder 制御
- ネイティブメソッド呼び出し調整

**実装ファイル**: `app/src/main/java/com/example/oblivion/GameActivity.kt`

```kotlin
package com.example.oblivion

import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.MotionEvent
import androidx.appcompat.app.AppCompatActivity

/**
 * Oblivion Game Activity
 * 
 * メインActivity。ゲームサーフェスの管理とライフサイクル制御を行う。
 */
class GameActivity : AppCompatActivity(), SurfaceHolder.Callback {
    
    private val tag = "GameActivity"
    private lateinit var gameView: SurfaceView
    private lateinit var inputHandler: InputHandler
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(tag, "onCreate()")
        
        // Full screen設定
        window.decorView.systemUiVisibility = (
            android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
            android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
            android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
            android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            android.view.View.SYSTEM_UI_FLAG_FULLSCREEN or
            android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        )
        
        // レイアウト設定
        setContentView(R.layout.activity_game)
        
        gameView = findViewById(R.id.game_surface)
        gameView.holder.addCallback(this)
        
        // 入力ハンドラー初期化
        inputHandler = InputHandler(this)
        
        // Engine 初期化
        try {
            OblivionEngine.initialize(this, assets)
            Log.d(tag, "Engine initialized successfully")
        } catch (e: OblivionException) {
            Log.e(tag, "Failed to initialize engine: ${e.message}")
            finishAffinity()
        }
    }
    
    override fun onResume() {
        super.onResume()
        Log.d(tag, "onResume()")
        
        try {
            OblivionEngine.resume()
        } catch (e: OblivionException) {
            Log.e(tag, "Error in onResume: ${e.message}")
        }
    }
    
    override fun onPause() {
        super.onPause()
        Log.d(tag, "onPause()")
        
        try {
            OblivionEngine.pause()
        } catch (e: OblivionException) {
            Log.e(tag, "Error in onPause: ${e.message}")
        }
    }
    
    override fun onDestroy() {
        Log.d(tag, "onDestroy()")
        
        try {
            OblivionEngine.destroy()
        } catch (e: OblivionException) {
            Log.e(tag, "Error in onDestroy: ${e.message}")
        }
        
        super.onDestroy()
    }
    
    // ====== SurfaceHolder.Callback Implementation ======
    
    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.d(tag, "surfaceCreated()")
        
        try {
            OblivionEngine.onSurfaceCreated(holder.surface)
        } catch (e: OblivionException) {
            Log.e(tag, "Error in surfaceCreated: ${e.message}")
        }
    }
    
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.d(tag, "surfaceChanged: $width x $height")
        
        try {
            OblivionEngine.onSurfaceChanged(width, height)
        } catch (e: OblivionException) {
            Log.e(tag, "Error in surfaceChanged: ${e.message}")
        }
    }
    
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.d(tag, "surfaceDestroyed()")
        
        try {
            OblivionEngine.onSurfaceDestroyed()
        } catch (e: OblivionException) {
            Log.e(tag, "Error in surfaceDestroyed: ${e.message}")
        }
    }
    
    // ====== タッチイベント処理 ======
    
    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if (event == null) return false
        
        val x = event.x
        val y = event.y
        val action = event.action
        
        try {
            when (action and MotionEvent.ACTION_MASK) {
                MotionEvent.ACTION_DOWN,
                MotionEvent.ACTION_MOVE,
                MotionEvent.ACTION_UP -> {
                    OblivionEngine.onTouchEvent(x, y, action)
                }
            }
        } catch (e: OblivionException) {
            Log.e(tag, "Error in onTouchEvent: ${e.message}")
        }
        
        return inputHandler.onTouchEvent(event)
    }
}
```

### 1.3 入力処理と Platform 機能

**InputHandler.kt** - タッチイベント管理

```kotlin
package com.example.oblivion

import android.view.MotionEvent
import android.app.Activity

/**
 * ゲーム入力を処理するハンドラー
 */
class InputHandler(private val activity: Activity) {
    
    fun onTouchEvent(event: MotionEvent): Boolean {
        return when (event.action and MotionEvent.ACTION_MASK) {
            MotionEvent.ACTION_DOWN -> onTouchDown(event)
            MotionEvent.ACTION_MOVE -> onTouchMove(event)
            MotionEvent.ACTION_UP -> onTouchUp(event)
            else -> false
        }
    }
    
    private fun onTouchDown(event: MotionEvent): Boolean {
        // マルチタッチ対応
        val pointerCount = event.pointerCount
        for (i in 0 until pointerCount) {
            val x = event.getX(i)
            val y = event.getY(i)
            val pointerId = event.getPointerId(i)
            // Engine に通知
            try {
                OblivionEngine.onTouchEvent(x, y, MotionEvent.ACTION_DOWN, pointerId)
            } catch (e: OblivionException) {
                // Handle error
            }
        }
        return true
    }
    
    private fun onTouchMove(event: MotionEvent): Boolean {
        val pointerCount = event.pointerCount
        for (i in 0 until pointerCount) {
            val x = event.getX(i)
            val y = event.getY(i)
            val pointerId = event.getPointerId(i)
            try {
                OblivionEngine.onTouchEvent(x, y, MotionEvent.ACTION_MOVE, pointerId)
            } catch (e: OblivionException) {
                // Handle error
            }
        }
        return true
    }
    
    private fun onTouchUp(event: MotionEvent): Boolean {
        val pointerCount = event.pointerCount
        for (i in 0 until pointerCount) {
            val x = event.getX(i)
            val y = event.getY(i)
            val pointerId = event.getPointerId(i)
            try {
                OblivionEngine.onTouchEvent(x, y, MotionEvent.ACTION_UP, pointerId)
            } catch (e: OblivionException) {
                // Handle error
            }
        }
        return true
    }
}
```

---

## C++ 側の JNI ブリッジ

### 2.1 JNI ネイティブメソッド実装

**ファイル**: `app/src/main/cpp/jni/com_example_oblivion_OblivionEngine.h`

```cpp
#ifndef COM_EXAMPLE_OBLIVION_OBLIVIONENGINE_H_
#define COM_EXAMPLE_OBLIVION_OBLIVIONENGINE_H_

#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <memory>
#include <mutex>

// Forward declarations
class Engine;

/**
 * JNI Bridge for OblivionEngine
 * 
 * Handles all JNI calls from Java/Kotlin to C++ Engine.
 * Manages lifecycle, data conversion, and exception handling.
 */
class OblivionEngineJNI {
public:
    /**
     * Initialize the native engine
     * Called from: OblivionEngine.nativeInit()
     * 
     * @param env JNI environment
     * @param assetManager Android AssetManager
     * @param cacheDir Cache directory path
     * @param assetDir Asset directory path
     * @return JNI_OK (0) on success, error code otherwise
     */
    static jint nativeInit(
        JNIEnv* env,
        jobject obj,
        jobject assetManager,
        jstring cacheDir,
        jstring assetDir
    );
    
    /**
     * Get native engine handle
     * @return Pointer to Engine cast as jlong
     */
    static jlong nativeGetHandle(JNIEnv* env, jobject obj);
    
    /**
     * Surface created callback
     * @param surface ANativeWindow (from Surface object)
     * @return JNI_OK on success
     */
    static jint nativeOnSurfaceCreated(
        JNIEnv* env,
        jobject obj,
        jobject surface
    );
    
    /**
     * Surface changed (resize) callback
     * @param width New width
     * @param height New height
     * @return JNI_OK on success
     */
    static jint nativeOnSurfaceChanged(
        JNIEnv* env,
        jobject obj,
        jint width,
        jint height
    );
    
    /**
     * Surface destroyed callback
     */
    static void nativeOnSurfaceDestroyed(JNIEnv* env, jobject obj);
    
    /**
     * Pause game loop
     */
    static void nativePause(JNIEnv* env, jobject obj);
    
    /**
     * Resume game loop
     */
    static void nativeResume(JNIEnv* env, jobject obj);
    
    /**
     * Handle touch event
     * @param x Touch X coordinate
     * @param y Touch Y coordinate
     * @param action MotionEvent action (ACTION_DOWN, ACTION_MOVE, ACTION_UP)
     * @param pointerId Pointer ID for multitouch
     */
    static void nativeOnTouchEvent(
        JNIEnv* env,
        jobject obj,
        jfloat x,
        jfloat y,
        jint action,
        jint pointerId
    );
    
    /**
     * Destroy and cleanup
     */
    static void nativeDestroy(JNIEnv* env, jobject obj);
    
private:
    // Static Engine instance (singleton)
    static std::unique_ptr<Engine> sEngine;
    static std::mutex sEngineMutex;
    
    /**
     * Convert JNI exception to Java Exception
     */
    static void ThrowJavaException(JNIEnv* env, const char* exceptionClass, const char* message);
    
    /**
     * Convert Java string to C++ string
     */
    static std::string JStringToString(JNIEnv* env, jstring str);
    
    /**
     * Convert Java exception to C++ exception
     */
    static void HandleJavaException(JNIEnv* env);
};

// Error codes
enum JNIErrorCode {
    JNI_OK = 0,
    JNI_ERR_NOT_INITIALIZED = -1,
    JNI_ERR_ALREADY_INITIALIZED = -2,
    JNI_ERR_INVALID_PARAMETER = -3,
    JNI_ERR_SURFACE_INVALID = -4,
    JNI_ERR_VULKAN_INIT = -5,
    JNI_ERR_OUT_OF_MEMORY = -6,
    JNI_ERR_UNKNOWN = -100
};

#endif // COM_EXAMPLE_OBLIVION_OBLIVIONENGINE_H_
```

**ファイル**: `app/src/main/cpp/jni/com_example_oblivion_OblivionEngine.cpp`

```cpp
#include "com_example_oblivion_OblivionEngine.h"
#include "../engine/Engine.h"
#include <android/log.h>
#include <exception>

#define LOG_TAG "OblivionEngineJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// Static member initialization
std::unique_ptr<Engine> OblivionEngineJNI::sEngine = nullptr;
std::mutex OblivionEngineJNI::sEngineMutex;

// JNI function registration
static const JNINativeMethod methods[] = {
    {"nativeInit", "(Landroid/content/res/AssetManager;Ljava/lang/String;Ljava/lang/String;)I",
     (void*)OblivionEngineJNI::nativeInit},
    {"nativeGetHandle", "()J", (void*)OblivionEngineJNI::nativeGetHandle},
    {"nativeOnSurfaceCreated", "(Landroid/view/Surface;)I",
     (void*)OblivionEngineJNI::nativeOnSurfaceCreated},
    {"nativeOnSurfaceChanged", "(II)I", (void*)OblivionEngineJNI::nativeOnSurfaceChanged},
    {"nativeOnSurfaceDestroyed", "()V", (void*)OblivionEngineJNI::nativeOnSurfaceDestroyed},
    {"nativePause", "()V", (void*)OblivionEngineJNI::nativePause},
    {"nativeResume", "()V", (void*)OblivionEngineJNI::nativeResume},
    {"nativeOnTouchEvent", "(FFII)V", (void*)OblivionEngineJNI::nativeOnTouchEvent},
    {"nativeDestroy", "()V", (void*)OblivionEngineJNI::nativeDestroy},
};

/**
 * JNI_OnLoad - Called when library is loaded
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGD("JNI_OnLoad called");
    
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        return JNI_ERR;
    }
    
    const char* className = "com/example/oblivion/OblivionEngine";
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        LOGE("Failed to find class: %s", className);
        return JNI_ERR;
    }
    
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        LOGE("Failed to register native methods");
        return JNI_ERR;
    }
    
    LOGD("Native methods registered successfully");
    return JNI_VERSION_1_6;
}

/**
 * nativeInit implementation
 */
jint OblivionEngineJNI::nativeInit(
    JNIEnv* env,
    jobject obj,
    jobject assetManager,
    jstring cacheDir,
    jstring assetDir) {
    
    LOGD("nativeInit called");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        // Check if already initialized
        if (sEngine != nullptr) {
            LOGD("Engine already initialized");
            return JNI_ERR_ALREADY_INITIALIZED;
        }
        
        // Convert Java strings to C++
        std::string cache_dir = JStringToString(env, cacheDir);
        std::string asset_dir = JStringToString(env, assetDir);
        
        // Get AAssetManager from jobject
        AAssetManager* asset_mgr = AAssetManager_fromJava(env, assetManager);
        if (asset_mgr == nullptr) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Failed to get AAssetManager");
            return JNI_ERR_INVALID_PARAMETER;
        }
        
        // Create and initialize engine
        sEngine = std::make_unique<Engine>();
        
        Engine::InitParams params;
        params.asset_manager = asset_mgr;
        params.cache_dir = cache_dir;
        params.asset_dir = asset_dir;
        
        if (!sEngine->init(params)) {
            LOGE("Engine initialization failed");
            sEngine = nullptr;
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine initialization failed");
            return JNI_ERR_VULKAN_INIT;
        }
        
        LOGD("Engine initialized successfully");
        return JNI_OK;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeInit: %s", e.what());
        ThrowJavaException(env, "com/example/oblivion/OblivionException", e.what());
        sEngine = nullptr;
        return JNI_ERR_UNKNOWN;
    } catch (...) {
        LOGE("Unknown exception in nativeInit");
        ThrowJavaException(env, "com/example/oblivion/OblivionException",
                         "Unknown error in nativeInit");
        sEngine = nullptr;
        return JNI_ERR_UNKNOWN;
    }
}

/**
 * nativeGetHandle implementation
 */
jlong OblivionEngineJNI::nativeGetHandle(JNIEnv* env, jobject obj) {
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    if (!sEngine) {
        return 0;
    }
    
    return reinterpret_cast<jlong>(sEngine.get());
}

/**
 * nativeOnSurfaceCreated implementation
 */
jint OblivionEngineJNI::nativeOnSurfaceCreated(
    JNIEnv* env,
    jobject obj,
    jobject surface) {
    
    LOGD("nativeOnSurfaceCreated called");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (!sEngine) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine not initialized");
            return JNI_ERR_NOT_INITIALIZED;
        }
        
        // Convert Surface to ANativeWindow
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Failed to get ANativeWindow from Surface");
            return JNI_ERR_SURFACE_INVALID;
        }
        
        if (!sEngine->onSurfaceCreated(window)) {
            ANativeWindow_release(window);
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine onSurfaceCreated failed");
            return JNI_ERR_VULKAN_INIT;
        }
        
        return JNI_OK;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeOnSurfaceCreated: %s", e.what());
        ThrowJavaException(env, "com/example/oblivion/OblivionException", e.what());
        return JNI_ERR_UNKNOWN;
    }
}

/**
 * nativeOnSurfaceChanged implementation
 */
jint OblivionEngineJNI::nativeOnSurfaceChanged(
    JNIEnv* env,
    jobject obj,
    jint width,
    jint height) {
    
    LOGD("nativeOnSurfaceChanged: %d x %d", width, height);
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (!sEngine) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine not initialized");
            return JNI_ERR_NOT_INITIALIZED;
        }
        
        if (width <= 0 || height <= 0) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Invalid surface dimensions");
            return JNI_ERR_INVALID_PARAMETER;
        }
        
        if (!sEngine->onSurfaceChanged(width, height)) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine onSurfaceChanged failed");
            return JNI_ERR_VULKAN_INIT;
        }
        
        return JNI_OK;
        
    } catch (const std::exception& e) {
        LOGE("Exception in nativeOnSurfaceChanged: %s", e.what());
        ThrowJavaException(env, "com/example/oblivion/OblivionException", e.what());
        return JNI_ERR_UNKNOWN;
    }
}

/**
 * nativeOnSurfaceDestroyed implementation
 */
void OblivionEngineJNI::nativeOnSurfaceDestroyed(JNIEnv* env, jobject obj) {
    LOGD("nativeOnSurfaceDestroyed called");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (sEngine) {
            sEngine->onSurfaceDestroyed();
        }
    } catch (const std::exception& e) {
        LOGE("Exception in nativeOnSurfaceDestroyed: %s", e.what());
    }
}

/**
 * nativePause implementation
 */
void OblivionEngineJNI::nativePause(JNIEnv* env, jobject obj) {
    LOGD("nativePause called");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (sEngine) {
            sEngine->pause();
        }
    } catch (const std::exception& e) {
        LOGE("Exception in nativePause: %s", e.what());
    }
}

/**
 * nativeResume implementation
 */
void OblivionEngineJNI::nativeResume(JNIEnv* env, jobject obj) {
    LOGD("nativeResume called");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (sEngine) {
            sEngine->resume();
        }
    } catch (const std::exception& e) {
        LOGE("Exception in nativeResume: %s", e.what());
    }
}

/**
 * nativeOnTouchEvent implementation
 */
void OblivionEngineJNI::nativeOnTouchEvent(
    JNIEnv* env,
    jobject obj,
    jfloat x,
    jfloat y,
    jint action,
    jint pointerId) {
    
    // Use read-only lock for quick operation
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (sEngine) {
            sEngine->onTouchEvent(x, y, action, pointerId);
        }
    } catch (const std::exception& e) {
        LOGE("Exception in nativeOnTouchEvent: %s", e.what());
    }
}

/**
 * nativeDestroy implementation
 */
void OblivionEngineJNI::nativeDestroy(JNIEnv* env, jobject obj) {
    LOGD("nativeDestroy called");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (sEngine) {
            sEngine->shutdown();
            sEngine = nullptr;
            LOGD("Engine destroyed successfully");
        }
    } catch (const std::exception& e) {
        LOGE("Exception in nativeDestroy: %s", e.what());
        sEngine = nullptr;
    }
}

/**
 * Helper: Throw Java Exception
 */
void OblivionEngineJNI::ThrowJavaException(
    JNIEnv* env,
    const char* exceptionClass,
    const char* message) {
    
    jclass exceptionClazz = env->FindClass(exceptionClass);
    if (exceptionClazz != nullptr) {
        env->ThrowNew(exceptionClazz, message);
        env->DeleteLocalRef(exceptionClazz);
    }
}

/**
 * Helper: Convert Java String to C++ String
 */
std::string OblivionEngineJNI::JStringToString(JNIEnv* env, jstring str) {
    if (str == nullptr) {
        return "";
    }
    
    const char* cstr = env->GetStringUTFChars(str, nullptr);
    std::string result(cstr);
    env->ReleaseStringUTFChars(str, cstr);
    
    return result;
}
```

### 2.2 Engine クラス定義

**ファイル**: `app/src/main/cpp/engine/Engine.h`

```cpp
#ifndef ENGINE_H_
#define ENGINE_H_

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <thread>

/**
 * Main Game Engine
 * 
 * Manages:
 * - Vulkan initialization and rendering
 * - Game update loop
 * - Asset management
 * - Subsystem coordination
 */
class Engine {
public:
    struct InitParams {
        AAssetManager* asset_manager;
        std::string cache_dir;
        std::string asset_dir;
    };
    
    Engine();
    ~Engine();
    
    // Lifecycle
    bool init(const InitParams& params);
    void shutdown();
    
    // Surface management
    bool onSurfaceCreated(ANativeWindow* window);
    bool onSurfaceChanged(int width, int height);
    void onSurfaceDestroyed();
    
    // Game loop control
    void pause();
    void resume();
    void update();
    
    // Input
    void onTouchEvent(float x, float y, int action, int pointerId);
    
    // Getters
    bool isRunning() const;
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
private:
    // Engine state
    enum class State {
        UNINITIALIZED,
        INITIALIZING,
        INITIALIZED,
        RUNNING,
        PAUSED,
        SHUTTING_DOWN,
        DESTROYED
    };
    
    State state_;
    
    // Surface and rendering
    ANativeWindow* window_;
    VkSurfaceKHR surface_;
    int width_;
    int height_;
    
    // Vulkan resources
    VkInstance instance_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue queue_;
    
    // Asset management
    AAssetManager* assetManager_;
    std::string cacheDir_;
    std::string assetDir_;
    
    // Render thread
    std::thread renderThread_;
    bool shouldRender_;
    
    // Internal initialization
    bool initializeVulkan();
    bool createSurface(ANativeWindow* window);
    void renderLoop();
    void cleanupVulkan();
};

#endif // ENGINE_H_
```

**ファイル**: `app/src/main/cpp/engine/Engine.cpp`

```cpp
#include "Engine.h"
#include <android/log.h>

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

Engine::Engine()
    : state_(State::UNINITIALIZED),
      window_(nullptr),
      surface_(VK_NULL_HANDLE),
      width_(0),
      height_(0),
      instance_(VK_NULL_HANDLE),
      physicalDevice_(VK_NULL_HANDLE),
      device_(VK_NULL_HANDLE),
      queue_(VK_NULL_HANDLE),
      assetManager_(nullptr),
      shouldRender_(false) {
    LOGD("Engine constructor called");
}

Engine::~Engine() {
    LOGD("Engine destructor called");
    shutdown();
}

bool Engine::init(const InitParams& params) {
    LOGD("Engine::init() called");
    
    if (state_ != State::UNINITIALIZED) {
        LOGE("Engine already initialized or in invalid state");
        return false;
    }
    
    state_ = State::INITIALIZING;
    
    // Store parameters
    assetManager_ = params.asset_manager;
    cacheDir_ = params.cache_dir;
    assetDir_ = params.asset_dir;
    
    if (!assetManager_) {
        LOGE("AssetManager is null");
        state_ = State::UNINITIALIZED;
        return false;
    }
    
    // Initialize Vulkan
    if (!initializeVulkan()) {
        LOGE("Vulkan initialization failed");
        cleanupVulkan();
        state_ = State::UNINITIALIZED;
        return false;
    }
    
    // TODO: Initialize game systems (audio, input, world, etc.)
    
    state_ = State::INITIALIZED;
    LOGD("Engine initialized successfully");
    
    return true;
}

void Engine::shutdown() {
    LOGD("Engine::shutdown() called");
    
    shouldRender_ = false;
    
    if (renderThread_.joinable()) {
        renderThread_.join();
    }
    
    cleanupVulkan();
    
    if (window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
    
    state_ = State::DESTROYED;
}

bool Engine::onSurfaceCreated(ANativeWindow* window) {
    LOGD("Engine::onSurfaceCreated()");
    
    if (!window) {
        LOGE("Invalid window");
        return false;
    }
    
    window_ = window;
    
    if (!createSurface(window)) {
        LOGE("Failed to create Vulkan surface");
        return false;
    }
    
    LOGD("Surface created successfully");
    return true;
}

bool Engine::onSurfaceChanged(int width, int height) {
    LOGD("Engine::onSurfaceChanged(%d, %d)", width, height);
    
    if (width <= 0 || height <= 0) {
        LOGE("Invalid surface dimensions: %d x %d", width, height);
        return false;
    }
    
    width_ = width;
    height_ = height;
    
    // Recreate swapchain with new dimensions
    // TODO: Implement swapchain recreation
    
    return true;
}

void Engine::onSurfaceDestroyed() {
    LOGD("Engine::onSurfaceDestroyed()");
    
    // Cleanup surface
    if (surface_ != VK_NULL_HANDLE) {
        // vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    
    if (window_) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
}

void Engine::pause() {
    LOGD("Engine::pause()");
    shouldRender_ = false;
    state_ = State::PAUSED;
}

void Engine::resume() {
    LOGD("Engine::resume()");
    state_ = State::RUNNING;
    shouldRender_ = true;
}

void Engine::update() {
    // Update game logic
    // TODO: Implement game update
}

void Engine::onTouchEvent(float x, float y, int action, int pointerId) {
    // Handle touch input
    // TODO: Route to input system
}

bool Engine::isRunning() const {
    return state_ == State::RUNNING;
}

bool Engine::initializeVulkan() {
    LOGD("Initializing Vulkan");
    
    // TODO: Create VkInstance
    // TODO: Select physical device
    // TODO: Create VkDevice
    // TODO: Create graphics queue
    
    return true;
}

bool Engine::createSurface(ANativeWindow* window) {
    LOGD("Creating Vulkan surface");
    
    // TODO: Create VkSurfaceKHR from ANativeWindow
    
    return true;
}

void Engine::renderLoop() {
    LOGD("Render loop started");
    
    while (shouldRender_) {
        if (state_ == State::RUNNING) {
            // TODO: Record Vulkan commands
            // TODO: Submit to queue
            // TODO: Present
        }
    }
    
    LOGD("Render loop ended");
}

void Engine::cleanupVulkan() {
    LOGD("Cleaning up Vulkan");
    
    // TODO: Destroy Vulkan objects
    
    instance_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
}
```

---

## 通信プロトコル

### 3.1 初期化時の通信フロー

```
Java App Start
    │
    └─► GameActivity.onCreate()
            │
            ├─► setContentView(R.layout.activity_game)
            ├─► gameView.holder.addCallback(this)
            │
            └─► OblivionEngine.initialize(context, assetManager)
                    │
                    └─► [JNI Call]
                            nativeInit(assetManager, cacheDir, assetDir)
                            │
                            ├─► Create Engine instance
                            ├─► Engine::init()
                            │   ├─► Initialize Vulkan instance
                            │   ├─► Load base assets
                            │   └─► Initialize subsystems
                            │
                            └─► Return: JNI_OK or error code
```

### 3.2 Surface ライフサイクル

```
GameActivity.onResume()
    │
    └─► SurfaceHolder.surfaceCreated(holder)
            │
            └─► OblivionEngine.onSurfaceCreated(surface)
                    │
                    └─► [JNI Call]
                            nativeOnSurfaceCreated(surface)
                            │
                            ├─► Convert Surface → ANativeWindow
                            ├─► Engine::onSurfaceCreated(window)
                            │   ├─► Create VkSurfaceKHR
                            │   ├─► Query surface capabilities
                            │   └─► Create swapchain
                            │
                            └─► Return: JNI_OK or error

SurfaceHolder.surfaceChanged(holder, format, width, height)
    │
    └─► OblivionEngine.onSurfaceChanged(width, height)
            │
            └─► [JNI Call]
                    nativeOnSurfaceChanged(width, height)
                    │
                    └─► Engine::onSurfaceChanged()
                        ├─► Recreate swapchain
                        └─► Update viewport/scissor
```

### 3.3 入力イベント伝達

```
GameActivity.onTouchEvent(event)
    │
    ├─► InputHandler.onTouchEvent(event)
    │
    └─► For each pointer:
            OblivionEngine.onTouchEvent(x, y, action, pointerId)
                │
                └─► [JNI Call]
                        nativeOnTouchEvent(x, y, action, pointerId)
                        │
                        └─► Engine::onTouchEvent()
                            └─► InputSystem::onTouch()
                                └─► Game logic / UI routing
```

### 3.4 データ構造定義

**タッチイベント構造体** (C++ 側)

```cpp
struct TouchEvent {
    float x;
    float y;
    int action;  // ACTION_DOWN, ACTION_MOVE, ACTION_UP
    int pointerId;
    uint64_t timestamp;
};
```

**エラーコード定義**

```cpp
enum JNIErrorCode {
    JNI_OK = 0,                    // Success
    JNI_ERR_NOT_INITIALIZED = -1,   // Engine not initialized
    JNI_ERR_ALREADY_INITIALIZED = -2, // Engine already initialized
    JNI_ERR_INVALID_PARAMETER = -3,  // Invalid parameter
    JNI_ERR_SURFACE_INVALID = -4,    // Surface invalid
    JNI_ERR_VULKAN_INIT = -5,        // Vulkan initialization failed
    JNI_ERR_OUT_OF_MEMORY = -6,      // Memory allocation failed
    JNI_ERR_UNKNOWN = -100           // Unknown error
};
```

---

## ライフサイクル管理

### 4.1 Engine ステートマシン

```
[UNINITIALIZED]
        │
        ├─ init()
        ↓
   [INITIALIZING]
        │
        ├─ (success)
        ↓
   [INITIALIZED] ◄─────────┐
        │                  │
        ├─ onSurfaceCreated()  │ onSurfaceDestroyed()
        ↓                  │
   [RUNNING] ───────────► [INITIALIZED]
        │
        ├─ pause()
        ↓
   [PAUSED] ◄──────┐
        │          │
        └─ resume()─┘
        │
        ├─ shutdown()
        ↓
   [DESTROYED]
```

### 4.2 VkSurfaceKHR ライフサイクル

```
初期状態: VK_NULL_HANDLE

onSurfaceCreated(window)
    └─► window_ = window
    └─► createSurface(window)
        └─► vkCreateAndroidSurfaceKHR()
        └─► surface_ = VkSurfaceKHR

onSurfaceChanged(width, height)
    └─► width_ = width, height_ = height
    └─► Recreate swapchain
    └─► Destroy old images
    └─► Create new VkImage objects

onSurfaceDestroyed()
    └─► vkDestroySurfaceKHR()
    └─► surface_ = VK_NULL_HANDLE
    └─► ANativeWindow_release()
```

---

## エラーハンドリング戦略

### 5.1 例外処理フロー

```cpp
// JNI 側で例外をキャッチ
try {
    // ネイティブコード実行
    bool result = engine->doSomething();
    
    if (!result) {
        ThrowJavaException(env, "com/example/oblivion/OblivionException",
                         "Operation failed");
        return JNI_ERR_UNKNOWN;
    }
    return JNI_OK;
    
} catch (const std::exception& e) {
    // 標準例外
    ThrowJavaException(env, "com/example/oblivion/OblivionException", e.what());
    return JNI_ERR_UNKNOWN;
    
} catch (...) {
    // 予期しない例外
    ThrowJavaException(env, "java/lang/RuntimeException",
                     "Unknown native error");
    return JNI_ERR_UNKNOWN;
}
```

### 5.2 Java 側での処理

```kotlin
try {
    OblivionEngine.initialize(this, assets)
} catch (e: OblivionException) {
    Log.e(TAG, "Initialization failed: ${e.message}")
    showErrorDialog("Failed to initialize engine: ${e.message}")
    finishAffinity()
}
```

### 5.3 デバッグ機能

**ログ出力**:
- C++ → `__android_log_print()`
- Java → `Log.d()`, `Log.e()`
- 統一タグ: `"OblivionEngine"`, `"OblivionEngineJNI"`, `"Engine"`

**デバッグフラグ**:
```cpp
#ifdef DEBUG_BUILD
#define DEBUG_LOG(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif
```

---

## メモリ管理

### 6.1 JNI メモリ管理ルール

**ローカルリファレンス**:
```cpp
// 自動削除される（256個まで）
jobject obj = env->NewObject(clazz, methodID);
```

**グローバルリファレンス**:
```cpp
// 明示的に削除する必要がある
jobject globalRef = env->NewGlobalRef(obj);
// ...使用...
env->DeleteGlobalRef(globalRef);
```

### 6.2 Engine リソース管理

```cpp
class Engine {
    std::unique_ptr<VulkanRenderer> renderer_;  // 自動破棄
    std::unique_ptr<AssetManager> assetMgr_;
    std::vector<VkImage> images_;               // 明示的に破棄
    
    ~Engine() {
        // unique_ptr は自動的にデストラクタを呼び出す
        // VkImage は明示的に破棄が必要
        for (auto& img : images_) {
            // vkDestroyImage()
        }
    }
};
```

### 6.3 メモリリーク防止チェックリスト

- [ ] `vkAlloc*` に対応する `vkFree*` がある
- [ ] `vkCreate*` に対応する `vkDestroy*` がある
- [ ] `env->NewGlobalRef()` に対応する `DeleteGlobalRef()` がある
- [ ] `AAsset_open()` に対応する `AAsset_close()` がある
- [ ] `ANativeWindow_acquire()` に対応する `ANativeWindow_release()` がある

---

## 実装フロー

### アクティビティ初期化フロー

```
┌─────────────────────────────────────────┐
│  GameActivity.onCreate()                 │
├─────────────────────────────────────────┤
│ 1. setContentView(R.layout.activity_game) │
│ 2. gameView = findViewById(R.id.game_surface)
│ 3. gameView.holder.addCallback(this)    │
│ 4. inputHandler = InputHandler(this)    │
│ 5. OblivionEngine.initialize()          │
│    └─► nativeInit()                     │
│        └─► Engine::init()               │
│            ├─► Vulkan instance create   │
│            ├─► Physical device select   │
│            └─► Subsystems init          │
│ [Success] → Ready for Surface Events    │
│ [Failure] → Show error → finishAffinity() │
└─────────────────────────────────────────┘
```

### レンダリングループフロー

```
┌─────────────────────────────────┐
│ Render Thread (Spawned)          │
├─────────────────────────────────┤
│ while (shouldRender_) {          │
│   if (state == RUNNING) {        │
│     1. Acquire next image        │
│     2. Record command buffer     │
│     3. Submit to queue           │
│     4. Present to display        │
│   } else {                       │
│     Wait or idle                 │
│   }                              │
│ }                                │
└─────────────────────────────────┘
```

### Pause/Resume フロー

```
GameActivity.onPause()
    ├─► OblivionEngine.pause()
    │   └─► nativePause()
    │       └─► Engine::pause()
    │           └─► shouldRender_ = false
    │               state_ = PAUSED
    │
    └─► Render thread waits

GameActivity.onResume()
    ├─► OblivionEngine.resume()
    │   └─► nativeResume()
    │       └─► Engine::resume()
    │           └─► state_ = RUNNING
    │               shouldRender_ = true
    │
    └─► Render thread continues
```

---

## テスト方針

### 7.1 ユニットテスト

```kotlin
// app/src/test/java/com/example/oblivion/OblivionEngineTest.kt

@RunWith(RobolectricTestRunner::class)
class OblivionEngineTest {
    
    @Test
    fun testEngineInitialization() {
        val context = RuntimeEnvironment.getApplication()
        val result = OblivionEngine.initialize(context, context.assets)
        
        assertTrue(result)
        assertTrue(OblivionEngine.isInitialized())
    }
    
    @Test
    fun testDoubleInitialization() {
        // Should not crash, just return true
        OblivionEngine.initialize(context, assets)
        val result = OblivionEngine.initialize(context, assets)
        
        assertTrue(result)
    }
    
    @Test
    fun testPauseResumeSequence() {
        OblivionEngine.initialize(context, assets)
        
        OblivionEngine.pause()
        OblivionEngine.resume()
        
        assertTrue(OblivionEngine.isInitialized())
    }
    
    @Test
    fun testCleanDestruction() {
        OblivionEngine.initialize(context, assets)
        OblivionEngine.destroy()
        
        assertFalse(OblivionEngine.isInitialized())
    }
}
```

### 7.2 統合テスト

**手動テストチェックリスト**:
- [ ] App 起動 → Engine 初期化完了
- [ ] Surface 作成 → Vulkan surface 作成
- [ ] 画面回転 → Surface changed 正常処理
- [ ] タッチイベント → Input system に到達
- [ ] 一時停止（Home キー） → Engine pause 正常
- [ ] 再開（アプリ再起動） → Engine resume 正常
- [ ] 破棄 → リソース完全解放

### 7.3 ストレステスト

```python
# scripts/stress_test.py
import subprocess
import time

def test_rapid_pause_resume():
    """Rapid pause/resume cycles"""
    for i in range(100):
        subprocess.run(['adb', 'shell', 'input', 'keyevent', 'KEYCODE_HOME'])
        time.sleep(0.1)
        subprocess.run(['adb', 'shell', 'am', 'start', 'com.example.oblivion/.GameActivity'])
        time.sleep(0.5)

def test_rotation_cycles():
    """Rapid screen rotations"""
    for i in range(50):
        subprocess.run(['adb', 'shell', 'settings', 'put', 'system', 'user_rotation', '0'])
        time.sleep(0.2)
        subprocess.run(['adb', 'shell', 'settings', 'put', 'system', 'user_rotation', '1'])
        time.sleep(0.2)

if __name__ == '__main__':
    test_rapid_pause_resume()
    test_rotation_cycles()
```

### 7.4 メモリリーク検出

```bash
# Enable memory profiling
adb shell setprop libc.debug.malloc 1

# Run valgrind or similar
valgrind --leak-check=full --show-leak-kinds=all ./oblivion
```

---

## まとめ

このJNIブリッジ設計は以下を実現します:

1. **堅牢性**: マルチスレッド環境での安全な通信
2. **スケーラビリティ**: 将来の機能追加に対応可能な構造
3. **保守性**: 明確な責務分離と例外処理
4. **パフォーマンス**: 不要なロックを最小化
5. **デバッグ性**: 詳細なログと状態追跡

---

**作成者**: Claude Code  
**最終更新**: 2026-06-06
