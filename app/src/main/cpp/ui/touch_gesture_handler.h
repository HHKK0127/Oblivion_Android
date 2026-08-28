#pragma once

#include <functional>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <android/log.h>

#define GESTURE_LOG_TAG "TouchGestureHandler"
#define GESTURE_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, GESTURE_LOG_TAG, __VA_ARGS__)
#define GESTURE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, GESTURE_LOG_TAG, __VA_ARGS__)
#define GESTURE_LOGW(...) __android_log_print(ANDROID_LOG_WARN, GESTURE_LOG_TAG, __VA_ARGS__)

/**
 * @brief ジェスチャータイプ
 */
enum class GestureType {
    NONE,
    SINGLE_TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN,
    PINCH_IN,
    PINCH_OUT,
    TWO_FINGER_SWIPE
};

/**
 * @brief ジェスチャーイベント構造体
 */
struct GestureEvent {
    GestureType type;
    glm::vec2 position;       // 開始位置
    glm::vec2 delta;          // 移動量（スワイプ方向）
    float magnitude;          // 強さ（ピンチ比率、スワイプ距離）
    int pointerCount;         // タッチ数
    float duration;           // 持続時間（秒）

    GestureEvent()
        : type(GestureType::NONE), position(0.0f, 0.0f), delta(0.0f, 0.0f),
          magnitude(0.0f), pointerCount(0), duration(0.0f) {}
};

/**
 * @brief ジェスチャーコールバック型
 */
using GestureCallback = std::function<void(const GestureEvent&)>;

/**
 * @brief タッチジェスチャー認識器
 *
 * Phase 43: マルチタッチジェスチャーを認識し、コールバックで通知する。
 * - シングルタップ: 選択/インタラクト
 * - ダブルタップ: 走り/ダッシュ
 * - ロングプレス: コンテキストメニュー
 * - スワイプ: カメラ回転
 * - ピンチ: ズーム（マップ画面）
 * - 2本指スワイプ: カメラ移動
 */
class TouchGestureHandler {
public:
    TouchGestureHandler();
    ~TouchGestureHandler() = default;

    /**
     * @brief ジェスチャー認識パラメータを設定
     */
    void setTapTimeout(float seconds) { tapTimeout = seconds; }
    void setDoubleTapTimeout(float seconds) { doubleTapTimeout = seconds; }
    void setLongPressTimeout(float seconds) { longPressTimeout = seconds; }
    void setSwipeThreshold(float pixels) { swipeThreshold = pixels; }
    void setPinchThreshold(float ratio) { pinchThreshold = ratio; }

    /**
     * @brief ジェスチャーコールバックを登録
     * @param type 登録するジェスチャータイプ
     * @param callback コールバック関数
     */
    void registerCallback(GestureType type, GestureCallback callback);

    /**
     * @brief 全ジェスチャー共通コールバックを登録
     */
    void registerAllCallback(GestureCallback callback);

    // === タッチ入力（UISystem から呼び出し） ===

    void onTouchDown(float x, float y, int pointerId);
    void onTouchMove(float x, float y, int pointerId);
    void onTouchUp(float x, float y, int pointerId);

    /**
     * @brief 毎フレーム更新（ジェスチャー判定のタイミング管理）
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime);

    /**
     * @brief 状態リセット
     */
    void reset();

    /**
     * @brief 有効/無効切替
     */
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    /**
     * @brief タッチポイント情報
     */
    struct TouchPoint {
        glm::vec2 startPos;
        glm::vec2 currentPos;
        float startTime;
        bool active;

        TouchPoint() : startPos(0.0f, 0.0f), currentPos(0.0f, 0.0f), startTime(0.0f), active(false) {}
    };

    // 設定パラメータ
    float tapTimeout;         // シングルタップ判定時間（秒）
    float doubleTapTimeout;   // ダブルタップ間隔（秒）
    float longPressTimeout;   // ロングプレス判定時間（秒）
    float swipeThreshold;     // スワイプ最小距離（ピクセル）
    float pinchThreshold;     // ピンチ判定比率変化

    // 状態
    bool enabled;
    float currentTime;

    // マルチタッチ管理（最大10本）
    static constexpr int MAX_POINTERS = 10;
    std::array<TouchPoint, MAX_POINTERS> pointers;
    int activePointerCount;

    // ダブルタップ検出用
    float lastTapTime;
    glm::vec2 lastTapPos;

    // ピンチ検出用
    float initialPinchDistance;
    float currentPinchDistance;

    // コールバック
    std::vector<std::pair<GestureType, GestureCallback>> callbacks;
    GestureCallback allCallback;

    // 内部ヘルパー
    void detectTap(int pointerId);
    void detectLongPress(int pointerId);
    void detectSwipe(int pointerId);
    void detectPinch();
    void detectTwoFingerSwipe();

    float getDistance(const glm::vec2& a, const glm::vec2& b) const;
    void fireGesture(const GestureEvent& event);
    int findActivePointerCount() const;
};
