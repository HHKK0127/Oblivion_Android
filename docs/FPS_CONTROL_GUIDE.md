# FPS Control ガイド

## 概要

Oblivion Android アプリに FPS（フレームレート）制御機能を追加しました。30、60、120 fps など、デバイスや設定に応じて FPS を自由に選択できます。

---

## C++ 側の実装

### Renderer クラスの新規メソッド

```cpp
// FPS を設定
void setTargetFPS(int fps);

// 現在のターゲット FPS を取得
int getTargetFPS() const;
```

### 使用例（C++）

```cpp
// 60 FPS に設定
renderer->setTargetFPS(60);

// 30 FPS に設定
renderer->setTargetFPS(30);

// 120 FPS に設定
renderer->setTargetFPS(120);

// 現在のターゲット FPS を取得
int fps = renderer->getTargetFPS();
LOGI("Target FPS: %d", fps);
```

### 動作原理

```cpp
// 1. フレーム開始時刻を記録
lastFrameTime = now()

// 2. ゲーム処理実行

// 3. フレーム終了後、経過時間を計算
elapsedTime = now() - lastFrameTime

// 4. ターゲット時間 (1000/fps ms) に達していない場合、スリープして FPS を制御
if (elapsedTime < frameTimeThreshold) {
    sleep(frameTimeThreshold - elapsedTime)
}
```

**ターゲット時間の計算**:
```
30 FPS  → 33.33 ms / frame
60 FPS  → 16.67 ms / frame
120 FPS → 8.33 ms / frame
```

---

## JNI（Java/Kotlin 連携）側の実装

### JNI メソッド

```cpp
// Java から C++ の setTargetFPS() を呼び出す
extern "C" void Java_com_example_oblivion_GameRenderer_nativeSetTargetFPS(
        JNIEnv* env, jobject obj, jint fps)

// Java から C++ の getTargetFPS() を呼び出す
extern "C" jint Java_com_example_oblivion_GameRenderer_nativeGetTargetFPS(
        JNIEnv* env, jobject obj)
```

### Kotlin/Java での利用

#### **1. GameRenderer クラスに native メソッドを追加**

```kotlin
// app/src/main/java/com/example/oblivion/GameRenderer.kt

class GameRenderer : GLSurfaceView.Renderer {
    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    // Native methods
    private external fun nativeSetTargetFPS(fps: Int)
    private external fun nativeGetTargetFPS(): Int

    // Public Java methods
    fun setTargetFPS(fps: Int) {
        nativeSetTargetFPS(fps)
    }

    fun getTargetFPS(): Int {
        return nativeGetTargetFPS()
    }
}
```

#### **2. MainActivity から FPS を制御**

```kotlin
// app/src/main/java/com/example/oblivion/MainActivity.kt

class MainActivity : AppCompatActivity() {
    private lateinit var gameRenderer: GameRenderer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        gameRenderer = GameRenderer(this)
        
        // デフォルト: 60 FPS
        gameRenderer.setTargetFPS(60)
    }

    // FPS を変更するボタンリスナー
    fun onFPS30Clicked() {
        gameRenderer.setTargetFPS(30)
        Toast.makeText(this, "FPS: 30", Toast.LENGTH_SHORT).show()
    }

    fun onFPS60Clicked() {
        gameRenderer.setTargetFPS(60)
        Toast.makeText(this, "FPS: 60", Toast.LENGTH_SHORT).show()
    }

    fun onFPS120Clicked() {
        gameRenderer.setTargetFPS(120)
        Toast.makeText(this, "FPS: 120", Toast.LENGTH_SHORT).show()
    }

    // 現在の FPS を表示
    fun onShowFPSClicked() {
        val currentFps = gameRenderer.getTargetFPS()
        Toast.makeText(this, "Current FPS: $currentFps", Toast.LENGTH_SHORT).show()
    }
}
```

#### **3. UI から FPS を選択できるようにする**

```xml
<!-- app/src/main/res/layout/activity_main.xml -->

<LinearLayout
    android:layout_width="match_parent"
    android:layout_height="wrap_content"
    android:orientation="horizontal"
    android:gravity="center">

    <Button
        android:id="@+id/btn_fps_30"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        android:text="30 FPS" />

    <Button
        android:id="@+id/btn_fps_60"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        android:text="60 FPS" />

    <Button
        android:id="@+id/btn_fps_120"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        android:text="120 FPS" />
</LinearLayout>
```

```kotlin
// MainActivity でボタンリスナーを設定
findViewById<Button>(R.id.btn_fps_30).setOnClickListener { onFPS30Clicked() }
findViewById<Button>(R.id.btn_fps_60).setOnClickListener { onFPS60Clicked() }
findViewById<Button>(R.id.btn_fps_120).setOnClickListener { onFPS120Clicked() }
```

---

## パフォーマンス計測

### Logcat ログ出力

```bash
# FPS 設定時のログ
D Renderer: Target FPS changed to: 60 (16.67 ms per frame)

# フレーム毎のログ
D Renderer: Frame rendered: deltaTime=0.017, FPS=60.0, Target FPS=60

# 5秒毎のパフォーマンスレポート
D PerformanceMonitor: ========== Performance Report ==========
D PerformanceMonitor: Total Frames: 300
D PerformanceMonitor: Current FPS: 60.0
D PerformanceMonitor: Average Frame Time: 16.67 ms
D PerformanceMonitor: Memory Usage: 45.3%
```

### FPS による消費電力の違い

| FPS | フレーム時間 | 電力消費 | 用途 |
|-----|------------|--------|------|
| 30  | 33.33 ms   | 低（最適） | バッテリー節約、低スペック機器 |
| 60  | 16.67 ms   | 中（標準） | 一般的なゲームプレイ |
| 120 | 8.33 ms    | 高（高速） | 高フレームレート対応デバイス |

---

## トラブルシューティング

### Q: FPS を高く設定しても効果がない

**A**: フレームレート制御は CPU スレッドの sleep による制御です。以下の場合は効果が限定的です：

1. **デバイスのリフレッシュレートが低い場合**
   - Android デバイスは 60 Hz リフレッシュレート が一般的
   - 120 FPS 設定は 60 Hz ディスプレイでは 60 FPS に制限される

2. **OpenGL ES の VSYNC が有効な場合**
   - GLSurfaceView デフォルトで VSYNC が有効
   - ディスプレイの最大リフレッシュレートに制限される

**解決方法**:
```cpp
// CMakeLists.txt でビルド最適化を追加
target_compile_options(native-lib PRIVATE -O3)
```

### Q: メモリが増加し続ける

**A**: フレームレート制御による影響ではなく、他の要因の可能性があります。

- PerformanceMonitor が 300 フレーム分のバッファを保持
- 定期的に logcat で メモリ使用率を確認
- 不要なアセットがメモリに残っていないか確認

---

## 推奨設定

### デバイス別推奨 FPS

| デバイス仕様 | 推奨 FPS | 理由 |
|-----------|--------|------|
| 低スペック（RAM 2GB） | 30 FPS | バッテリー節約、メモリ効率 |
| 標準仕様（RAM 4-6GB） | 60 FPS | バランス型、標準的なゲーム体験 |
| 高スペック（RAM 8GB+） | 60-120 FPS | 滑らかな操作感 |

### バッテリー節約モード

```kotlin
// 低バッテリー時に自動で 30 FPS に切り替え
fun setupBatteryOptimization() {
    val batteryManager = getSystemService(Context.BATTERY_SERVICE) as BatteryManager
    val level = batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER)
    
    if (level < 20) {  // 20% 以下
        gameRenderer.setTargetFPS(30)
    }
}
```

---

## 実装仕様

### Renderer メンバー変数

```cpp
int targetFPS;                    // ターゲット FPS (デフォルト: 60)
float frameTimeThreshold;         // フレームごとの時間（ms）
std::chrono::high_resolution_clock::time_point lastFrameTime;  // 前フレームの時刻
```

### フレームレート制御の精度

- **計測精度**: マイクロ秒単位（chrono::high_resolution_clock）
- **スリープ精度**: ミリ秒単位（std::this_thread::sleep_for）
- **実際の FPS 誤差**: ±2% 程度（OS スケジューリング依存）

---

## まとめ

✅ FPS を 30/60/120 など自由に設定可能  
✅ JNI を通じて Kotlin/Java から制御可能  
✅ バッテリー最適化に対応  
✅ PerformanceMonitor と統合して詳細なログ出力  

---

*最終更新: 2026-04-17*
