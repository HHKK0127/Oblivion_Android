# JNI クイックリファレンス

## コマンドライン操作

### ビルド

```bash
# Android Studio から
./gradlew clean build

# または NDK を直接使用
cd app
ndk-build

# リリースビルド（最適化有効）
./gradlew clean assembleRelease
```

### テスト・デバッグ

```bash
# ログを表示
adb logcat -s OblivionEngine OblivionEngineJNI Engine

# タグ別ログフィルター
adb logcat tag:OblivionEngine

# リアルタイム logcat
adb logcat -v threadtime | grep -E "(OblivionEngine|crash)"

# ネイティブクラッシュを確認
adb logcat | grep -i "signal\|fault\|crash"
```

### デバイス操作

```bash
# App を起動
adb shell am start -n com.example.oblivion/.GameActivity

# App を停止
adb shell am force-stop com.example.oblivion

# 画面を回転
adb shell settings put system accelerometer_rotation 1
adb shell settings put system user_rotation 0  # Portrait
adb shell settings put system user_rotation 1  # Landscape

# メモリ情報
adb shell dumpsys meminfo com.example.oblivion
```

---

## JNI 関数テンプレート

### 基本的なテンプレート

```cpp
JNIEXPORT jint JNICALL Java_com_example_oblivion_OblivionEngine_nativeFunction(
    JNIEnv* env,
    jobject obj,
    jint parameter1,
    jstring parameter2) {
    
    try {
        // 初期化チェック
        std::lock_guard<std::mutex> lock(sEngineMutex);
        if (!sEngine) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine not initialized");
            return JNI_ERR_NOT_INITIALIZED;
        }
        
        // Java 文字列を C++ に変換
        std::string str = JStringToString(env, parameter2);
        
        // 処理
        bool result = sEngine->doSomething(parameter1, str);
        
        if (!result) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Operation failed");
            return JNI_ERR_UNKNOWN;
        }
        
        return JNI_OK;
        
    } catch (const std::exception& e) {
        LOGE("Exception: %s", e.what());
        ThrowJavaException(env, "java/lang/RuntimeException", e.what());
        return JNI_ERR_UNKNOWN;
    }
}
```

### Surface ハンドリングテンプレート

```cpp
JNIEXPORT jint JNICALL Java_com_example_oblivion_OblivionEngine_nativeOnSurfaceCreated(
    JNIEnv* env,
    jobject obj,
    jobject surface) {
    
    LOGD("nativeOnSurfaceCreated");
    
    std::lock_guard<std::mutex> lock(sEngineMutex);
    
    try {
        if (!sEngine) return JNI_ERR_NOT_INITIALIZED;
        
        // Java Surface を ANativeWindow に変換
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        if (!window) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Failed to get ANativeWindow");
            return JNI_ERR_SURFACE_INVALID;
        }
        
        // Engine に通知
        if (!sEngine->onSurfaceCreated(window)) {
            ANativeWindow_release(window);
            return JNI_ERR_VULKAN_INIT;
        }
        
        return JNI_OK;
        
    } catch (const std::exception& e) {
        LOGE("Exception: %s", e.what());
        ThrowJavaException(env, "com/example/oblivion/OblivionException", e.what());
        return JNI_ERR_UNKNOWN;
    }
}
```

---

## Kotlin コード スニペット

### Engine 初期化

```kotlin
try {
    val context = this  // Activity context
    val assetManager = assets
    
    OblivionEngine.initialize(context, assetManager)
    Log.d(TAG, "Engine initialized")
} catch (e: OblivionException) {
    Log.e(TAG, "Failed to initialize: ${e.message}")
    showErrorDialog(e.message)
    finish()
}
```

### Surface 管理

```kotlin
override fun surfaceCreated(holder: SurfaceHolder) {
    Log.d(TAG, "surfaceCreated")
    try {
        OblivionEngine.onSurfaceCreated(holder.surface)
    } catch (e: OblivionException) {
        Log.e(TAG, "Error: ${e.message}")
    }
}

override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
    Log.d(TAG, "surfaceChanged: $width x $height")
    try {
        OblivionEngine.onSurfaceChanged(width, height)
    } catch (e: OblivionException) {
        Log.e(TAG, "Error: ${e.message}")
    }
}
```

### ライフサイクル管理

```kotlin
override fun onResume() {
    super.onResume()
    try {
        OblivionEngine.resume()
    } catch (e: OblivionException) {
        Log.e(TAG, "Resume failed: ${e.message}")
    }
}

override fun onPause() {
    super.onPause()
    try {
        OblivionEngine.pause()
    } catch (e: OblivionException) {
        Log.e(TAG, "Pause failed: ${e.message}")
    }
}

override fun onDestroy() {
    try {
        OblivionEngine.destroy()
    } catch (e: OblivionException) {
        Log.e(TAG, "Destroy failed: ${e.message}")
    }
    super.onDestroy()
}
```

### タッチ入力

```kotlin
override fun onTouchEvent(event: MotionEvent?): Boolean {
    if (event == null) return false
    
    val x = event.x
    val y = event.y
    val action = event.action and MotionEvent.ACTION_MASK
    
    when (action) {
        MotionEvent.ACTION_DOWN,
        MotionEvent.ACTION_MOVE,
        MotionEvent.ACTION_UP -> {
            try {
                OblivionEngine.onTouchEvent(x, y, action)
            } catch (e: OblivionException) {
                Log.e(TAG, "Touch event failed: ${e.message}")
            }
        }
    }
    return true
}
```

---

## デバッグマクロ

### C++ ログマクロ

```cpp
#include <android/log.h>

#define LOG_TAG "OblivionEngine"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 使用例
LOGD("Width: %d, Height: %d", width, height);
LOGE("Failed to initialize: %s", errorMsg.c_str());
```

### アサーション

```cpp
#define ASSERT(condition) \
    if (!(condition)) { \
        LOGE("Assertion failed: %s at %s:%d", #condition, __FILE__, __LINE__); \
        abort(); \
    }

// 使用例
ASSERT(sEngine != nullptr);
ASSERT(width > 0 && height > 0);
```

### パフォーマンス計測

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// 処理

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
LOGD("Operation took %lld ms", duration.count());
```

---

## JNI データ型変換

### Java String ↔ C++ std::string

```cpp
// Java → C++
std::string JStringToString(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) return "";
    const char* cstr = env->GetStringUTFChars(jstr, nullptr);
    std::string result(cstr);
    env->ReleaseStringUTFChars(jstr, cstr);
    return result;
}

// C++ → Java
jstring StringToJString(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}
```

### Java配列 ↔ C++ vector

```cpp
// Java int[] → C++ vector
std::vector<int> JArrayToVector(JNIEnv* env, jintArray jarray) {
    jint* elements = env->GetIntArrayElements(jarray, nullptr);
    jsize length = env->GetArrayLength(jarray);
    std::vector<int> result(elements, elements + length);
    env->ReleaseIntArrayElements(jarray, elements, JNI_ABORT);
    return result;
}

// C++ vector → Java int[]
jintArray VectorToJArray(JNIEnv* env, const std::vector<int>& vec) {
    jintArray jarray = env->NewIntArray(vec.size());
    env->SetIntArrayRegion(jarray, 0, vec.size(), vec.data());
    return jarray;
}
```

### Java Object → C++ pointer

```cpp
// Java Object をポインタに変換
void* GetPointerFromObject(JNIEnv* env, jobject obj) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID fieldID = env->GetFieldID(clazz, "nativeHandle", "J");
    return (void*)env->GetLongField(obj, fieldID);
}

// ポインタを Java Object に設定
void SetPointerToObject(JNIEnv* env, jobject obj, void* ptr) {
    jclass clazz = env->GetObjectClass(obj);
    jfieldID fieldID = env->GetFieldID(clazz, "nativeHandle", "J");
    env->SetLongField(obj, fieldID, (jlong)ptr);
}
```

---

## エラーコードリファレンス

```cpp
enum JNIErrorCode {
    JNI_OK = 0,                           // 成功
    JNI_ERR_NOT_INITIALIZED = -1,         // Engine が初期化されていない
    JNI_ERR_ALREADY_INITIALIZED = -2,     // Engine が既に初期化されている
    JNI_ERR_INVALID_PARAMETER = -3,       // 無効なパラメータ
    JNI_ERR_SURFACE_INVALID = -4,         // Surface が無効
    JNI_ERR_VULKAN_INIT = -5,             // Vulkan 初期化失敗
    JNI_ERR_OUT_OF_MEMORY = -6,           // メモリ不足
    JNI_ERR_UNKNOWN = -100                // 不明なエラー
};
```

---

## よく使う adb コマンド

```bash
# 現在のアプリケーション情報
adb shell dumpsys window | grep -A 5 "mCurrentFocus"

# CPU 使用率
adb shell top -n 1 | grep oblivion

# GPU 使用率（Vulkan ドライバ依存）
adb shell dumpsys gpu | grep -A 10 "GPU"

# メモリダンプ
adb shell am dump-heap com.example.oblivion /data/local/tmp/heap.hprof

# スタックトレース取得
adb shell am dumpstack com.example.oblivion > stack.txt

# 一時停止・再開（プロセス制御）
adb shell kill -STOP $(adb shell pidof com.example.oblivion)
adb shell kill -CONT $(adb shell pidof com.example.oblivion)
```

---

## リソース参照

### 公式ドキュメント
- [Android JNI ガイド](https://developer.android.com/training/articles/on-device-debugging)
- [Vulkan API](https://www.khronos.org/vulkan/)
- [Android NDK ドキュメント](https://developer.android.com/ndk)

### デバッグツール
- `logcat`: Android のアプリケーションログ
- `Android Profiler`: CPU, メモリ, GPU 解析
- `LLDB`: デバッガー
- `systrace`: パフォーマンス解析

### 開発環境
- **Android Studio**: IDE
- **NDK r26**: ネイティブ開発キット
- **Gradle**: ビルドシステム
- **CMake**: C++ ビルドシステム

---

**最終更新**: 2026-06-06
