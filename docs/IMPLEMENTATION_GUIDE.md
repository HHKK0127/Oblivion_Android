# Oblivion Android - JNI 実装ガイド

**バージョン**: 1.0  
**最終更新**: 2026-06-06

---

## 目次

1. [セットアップ](#セットアップ)
2. [JNI 関数シグネチャ](#jni-関数シグネチャ)
3. [スレッドセーフティ戦略](#スレッドセーフティ戦略)
4. [デバッグテクニック](#デバッグテクニック)
5. [ベストプラクティス](#ベストプラクティス)
6. [トラブルシューティング](#トラブルシューティング)

---

## セットアップ

### CMakeLists.txt 設定例

```cmake
# app/src/main/cpp/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(oblivion-engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)

# Source files
set(SOURCES
    jni/com_example_oblivion_OblivionEngine.cpp
    engine/Engine.cpp
    # Add other subsystems as needed
)

# Create shared library
add_library(oblivion-engine SHARED ${SOURCES})

# Include directories
target_include_directories(oblivion-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/jni
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(oblivion-engine
    android
    log
    Vulkan::Vulkan
)

# Optional: Add optimizations
if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(oblivion-engine PRIVATE -O3)
endif()
```

### build.gradle 設定例

```gradle
// app/build.gradle

android {
    compileSdk 34
    
    ndkVersion "26.0.10665949"
    
    defaultConfig {
        applicationId "com.example.oblivion"
        minSdk 24
        targetSdk 34
        versionCode 1
        versionName "1.0"
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.22.1"
        }
    }
    
    packagingOptions {
        exclude 'lib/arm64-v8a/libc++_shared.so'
    }
}

dependencies {
    implementation 'androidx.appcompat:appcompat:1.6.1'
    implementation 'androidx.constraintlayout:constraintlayout:2.1.4'
    
    // Testing
    testImplementation 'junit:junit:4.13.2'
    testImplementation 'org.robolectric:robolectric:4.9'
    androidTestImplementation 'androidx.test.ext:junit:1.1.5'
    androidTestImplementation 'androidx.test.espresso:espresso-core:3.5.1'
}
```

---

## JNI 関数シグネチャ

### JNI データ型マッピング

| Java Type | JNI Type | C Type |
|-----------|----------|--------|
| boolean | jboolean | unsigned char (1 byte) |
| byte | jbyte | signed char |
| char | jchar | unsigned short |
| short | jshort | short |
| int | jint | long |
| long | jlong | long long |
| float | jfloat | float |
| double | jdouble | double |
| Object | jobject | pointer (void*) |
| String | jstring | pointer (void*) |
| byte[] | jbyteArray | pointer (void*) |

### メソッドシグネチャ形式

```
(InputTypes)ReturnType

例:
- (Landroid/content/res/AssetManager;Ljava/lang/String;Ljava/lang/String;)I
  → AssetManager, String, String を受け取り、int を返す

- (Landroid/view/Surface;)I
  → Surface を受け取り、int を返す

- (II)I
  → int, int を受け取り、int を返す

- (FFII)V
  → float, float, int, int を受け取り、void を返す
```

### シグネチャ生成ツール

```bash
# javah コマンドでシグネチャを自動生成
cd app/src/main/java
javah -d ../../main/cpp/jni com.example.oblivion.OblivionEngine

# または Java のリフレクション API を使用:
Method method = OblivionEngine.class.getDeclaredMethod("nativeInit", 
    AssetManager.class, String.class, String.class);
String signature = Type.getMethodDescriptor(method);
```

---

## スレッドセーフティ戦略

### 1. Mutex を使用したロック

```cpp
// engine/Engine.h
class Engine {
private:
    mutable std::mutex stateMutex_;
    State state_;
    
public:
    bool isRunning() const {
        std::lock_guard<std::mutex> lock(stateMutex_);
        return state_ == State::RUNNING;
    }
};
```

### 2. Read-Write Lock（読み取り多重化）

より効率的な実装:

```cpp
#include <shared_mutex>

class Engine {
private:
    mutable std::shared_mutex rwMutex_;
    State state_;
    
public:
    bool isRunning() const {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        return state_ == State::RUNNING;
    }
    
    void setState(State newState) {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        state_ = newState;
    }
};
```

### 3. Condition Variable によるスレッド同期

```cpp
class Engine {
private:
    std::mutex renderMutex_;
    std::condition_variable renderCV_;
    bool shouldRender_ = false;
    
public:
    void pause() {
        std::unique_lock<std::mutex> lock(renderMutex_);
        shouldRender_ = false;
    }
    
    void resume() {
        std::unique_lock<std::mutex> lock(renderMutex_);
        shouldRender_ = true;
        renderCV_.notify_all();  // Notify render thread
    }
    
private:
    void renderLoop() {
        while (true) {
            std::unique_lock<std::mutex> lock(renderMutex_);
            
            // Wait until shouldRender_ is true
            renderCV_.wait(lock, [this] { return shouldRender_; });
            
            // Render frame here
        }
    }
};
```

### 4. JNI スレッド安全性チェックリスト

- [ ] グローバルリファレンスはスレッドセーフ（JVM が管理）
- [ ] 各スレッドは独自の JNIEnv を持つ（JVM が提供）
- [ ] ネイティブ側で使用するデータ構造はスレッドセーフか？
- [ ] すべてのメンバ変数へのアクセスがロック保護されているか？

---

## デバッグテクニック

### 1. ログ出力戦略

**C++ 側**:

```cpp
#define LOG_TAG "OblivionEngine"
#define LOGV(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)

// 使用例
LOGD("Engine initialized with %d x %d surface", width, height);
```

**Kotlin 側**:

```kotlin
Log.d(TAG, "Message")
Log.e(TAG, "Error message", exception)
```

### 2. Android Studio logcat フィルター

```
# タグでフィルター
tag:OblivionEngine

# レベルでフィルター
level:ERROR

# 複合フィルター
tag:OblivionEngine level:ERROR
```

### 3. メモリリーク検出

```bash
# 実行中のメモリ使用量を監視
adb shell dumpsys meminfo com.example.oblivion

# デトール（Detours）でメモリ割り当てをトレース
adb shell setprop libc.debug.malloc 1
adb shell setprop libc.debug.malloc.program ./app

# 終了後にログを確認
adb shell getprop libc.debug.malloc
```

### 4. Vulkan 検証レイヤーの有効化

```bash
# Vulkan 検証レイヤーを有効化（デバッグビルドのみ）
adb shell setprop debug.vulkan.layers com.lunarg.gfxreconstruct

# または開発設定で有効化
adb shell settings put global debug_vulkan_layers 1
```

### 5. JNI クラッシュ解析

```bash
# スタックトレースを取得
adb logcat -d | grep -A 20 "FATAL"

# symbols ファイルでデコード
adb pull /system/lib64/libc++.so
adb pull /path/to/compiled/oblivion-engine.so

# addr2line でシンボルを解決
arm-linux-android-addr2line -e oblivion-engine.so 0x1234abcd
```

---

## ベストプラクティス

### 1. JNI 戻り値の安全性

```cpp
// 良い例: エラーコードを返す
JNIEXPORT jint JNICALL Java_com_example_oblivion_OblivionEngine_nativeInit(
    JNIEnv* env, jobject obj, ...) {
    
    try {
        if (!engine) {
            ThrowJavaException(env, "com/example/oblivion/OblivionException",
                             "Engine not initialized");
            return JNI_ERR_NOT_INITIALIZED;
        }
        return JNI_OK;
    } catch (const std::exception& e) {
        ThrowJavaException(env, "java/lang/RuntimeException", e.what());
        return JNI_ERR_UNKNOWN;
    }
}

// 悪い例: null を返す
JNIEXPORT jstring JNICALL ...nativeGetString(...) {
    return nullptr;  // エラーハンドリングが曖昧
}
```

### 2. リソースリーク防止

```cpp
// RAII パターンを使用
class SurfaceGuard {
private:
    VkSurfaceKHR surface_;
    VkInstance instance_;
    
public:
    SurfaceGuard(VkInstance inst, ANativeWindow* window) 
        : surface_(VK_NULL_HANDLE), instance_(inst) {
        VkAndroidSurfaceCreateInfoKHR info{};
        info.window = window;
        vkCreateAndroidSurfaceKHR(instance_, &info, nullptr, &surface_);
    }
    
    ~SurfaceGuard() {
        if (surface_ != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
        }
    }
    
    VkSurfaceKHR get() const { return surface_; }
};
```

### 3. パフォーマンス最適化

**ホットパス（頻繁に呼ばれる関数）でのロック最小化**:

```cpp
// 非効率: 毎フレーム（60回/秒）ロック競合
void onTouchEvent(float x, float y, int action) {
    std::lock_guard<std::mutex> lock(engineMutex_);
    // 重い処理
}

// 効率的: ロックフリー読み取り
void onTouchEvent(float x, float y, int action) {
    if (!engine_) return;  // volatile読取でOK
    engine_->processTouch(x, y, action);  // ロック内で軽い処理
}
```

### 4. 例外安全性

```cpp
// Exception Safe: RAII を使用
bool Engine::onSurfaceCreated(ANativeWindow* window) {
    auto guard = std::make_unique<SurfaceGuard>(instance_, window);
    
    // 処理
    
    // エラーが発生しても guard が破棄時にクリーンアップ
    return true;
}

// Exception Unsafe: 手動管理
bool Engine::onSurfaceCreated(ANativeWindow* window) {
    VkSurfaceKHR surface;
    vkCreateAndroidSurfaceKHR(..., &surface);
    
    if (somethingFails()) {
        return false;  // surface がリークする！
    }
    
    return true;
}
```

### 5. JNIEnv の正しい使い方

```cpp
// 良い例: ローカルリファレンス管理
void ThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass clazz = env->FindClass(className);
    if (clazz != nullptr) {
        env->ThrowNew(clazz, msg);
        env->DeleteLocalRef(clazz);  // 明示的に削除
    }
}

// グローバルリファレンス（長期保持が必要な場合のみ）
class CachedJavaClass {
private:
    static jobject exceptionClass_;
    
public:
    static void initialize(JNIEnv* env) {
        jclass localRef = env->FindClass("com/example/oblivion/OblivionException");
        exceptionClass_ = env->NewGlobalRef(localRef);
        env->DeleteLocalRef(localRef);
    }
    
    static void cleanup(JNIEnv* env) {
        env->DeleteGlobalRef(exceptionClass_);
        exceptionClass_ = nullptr;
    }
};
```

---

## トラブルシューティング

### よくあるエラー

#### 1. UnsatisfiedLinkError

```
UnsatisfiedLinkError: dlopen failed: cannot locate symbol "Java_com_example_oblivion_OblivionEngine_nativeInit"
```

**原因**:
- JNI 関数の命名規則が間違っている
- .so ファイルがビルドされていない
- .so ファイルが正しいディレクトリにない

**解決策**:
```bash
# ビルドして確認
./gradlew clean ndkBuild

# .so ファイルが存在するか確認
find . -name "*.so" -type f

# シンボルが正しいか確認
nm -D app/build/intermediates/cmake/release/lib/arm64-v8a/liboblivion-engine.so | grep nativeInit
```

#### 2. AndroidRuntime: java.lang.NullPointerException

```
AndroidRuntime: java.lang.NullPointerException: Attempt to invoke virtual method 'void com.example.oblivion.OblivionEngine.nativePause()' on a null object reference
```

**原因**:
- Engine がまだ初期化されていない状態でメソッドが呼ばれた
- ネイティブメソッドが null を返している

**解決策**:
```kotlin
// 初期化チェック
if (OblivionEngine.isInitialized()) {
    OblivionEngine.pause()
}

// JNI 側で state をチェック
if (!sEngine) {
    ThrowJavaException(env, "OblivionException", "Not initialized");
    return;
}
```

#### 3. Vulkan: VK_ERROR_SURFACE_LOST_KHR

```
Vulkan validation error: vkQueuePresentKHR() returned VK_ERROR_SURFACE_LOST_KHR
```

**原因**:
- Surface が破棄されたが、rendering thread がまだ使用しようとしている
- Surface のライフサイクル管理が不適切

**解決策**:
```cpp
void renderLoop() {
    while (shouldRender_) {
        if (!surface_ || surface_ == VK_NULL_HANDLE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // Render only if surface is valid
    }
}
```

#### 4. デッドロック

```
ANR (Application Not Responding) - main thread が locked
```

**原因**:
- JNI 呼び出し内で別のロックを取得しようとしている
- 循環ロック依存

**解決策**:
```cpp
// 悪い例: ロック順序が一貫していない
void methodA() {
    lock(mutexA);
    lock(mutexB);  // A→B
}

void methodB() {
    lock(mutexB);
    lock(mutexA);  // B→A (デッドロック!)
}

// 良い例: 常に同じ順序
void methodA() {
    lock(mutexA);
    lock(mutexB);  // A→B
}

void methodB() {
    lock(mutexA);
    lock(mutexB);  // A→B
}
```

---

## チェックリスト

### ビルド前

- [ ] CMakeLists.txt が正しく設定されているか
- [ ] NDK バージョンが指定されているか
- [ ] Vulkan SDK がインストールされているか
- [ ] JNI ヘッダーが include されているか

### テスト前

- [ ] すべてのネイティブメソッドがロック保護されているか
- [ ] 例外ハンドリングが実装されているか
- [ ] リソースリークテストを実施したか
- [ ] デバイスのメモリが十分か

### 本番運用前

- [ ] ストレステスト（100+回の pause/resume）を実施したか
- [ ] 画面回転時の挙動をテストしたか
- [ ] 極端な環境（低メモリ、高温）でのテストを実施したか
- [ ] Vulkan 検証レイヤーで警告がないか

---

**作成者**: Claude Code  
**最終更新**: 2026-06-06
