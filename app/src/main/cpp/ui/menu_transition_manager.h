#pragma once

#include <string>
#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include <android/log.h>

#define MENU_TRANS_LOG_TAG "MenuTransitionManager"
#define MTRANS_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MENU_TRANS_LOG_TAG, __VA_ARGS__)
#define MTRANS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, MENU_TRANS_LOG_TAG, __VA_ARGS__)

class TextRenderer;

/**
 * @brief メニュー遷移タイプ
 */
enum class TransitionType {
    FADE_IN,
    FADE_OUT,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN,
    NONE
};

/**
 * @brief 遷移状態
 */
enum class TransitionState {
    IDLE,
    OUTGOING,   // 遷移元が退出中
    INCOMING    // 遷移先が登場中
};

/**
 * @brief メニュー遷移アニメーション管理
 *
 * Phase 43: メニュー間のスムーズな遷移アニメーションを提供。
 * - フェードイン/アウト
 * - スライド遷移（左/右/上/下）
 * - 遷移中の入力ブロック
 * - イージング関数対応
 */
class MenuTransitionManager {
public:
    MenuTransitionManager();
    ~MenuTransitionManager() = default;

    /**
     * @brief 初期化
     * @param textRenderer テキストレンダラー
     * @param screenWidth スクリーン幅
     * @param screenHeight スクリーン高さ
     */
    void initialize(TextRenderer* textRenderer, int screenWidth, int screenHeight);

    /**
     * @brief 毎フレーム更新
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime);

    /**
     * @brief 遷移オーバーレイを描画
     */
    void render();

    /**
     * @brief 遷移を開始
     * @param type 遷移タイプ
     * @param duration 遷移時間（秒）
     * @param onOutgoingComplete 遷移元退出完了コールバック
     * @param onIncomingComplete 遷移先登場完了コールバック
     */
    void startTransition(TransitionType type, float duration,
                         std::function<void()> onOutgoingComplete = nullptr,
                         std::function<void()> onIncomingComplete = nullptr);

    /**
     * @brief 遷移中かどうか
     */
    bool isTransitioning() const { return state != TransitionState::IDLE; }

    /**
     * @brief 入力ブロック中かどうか
     */
    bool isInputBlocked() const { return state != TransitionState::IDLE; }

    /**
     * @brief 現在の遷移アルファ値を取得（0.0〜1.0）
     */
    float getCurrentAlpha() const { return currentAlpha; }

    /**
     * @brief スクリーンサイズ変更
     */
    void setScreenSize(int width, int height);

    /**
     * @brief 遷移速度を設定（デフォルト: 0.3秒）
     */
    void setDefaultDuration(float seconds) { defaultDuration = seconds; }

    /**
     * @brief オーバーレイ色を設定
     */
    void setOverlayColor(const glm::vec4& color) { overlayColor = color; }

private:
    TextRenderer* textRenderer;
    int screenWidth;
    int screenHeight;

    // 遷移状態
    TransitionState state;
    TransitionType currentType;
    float transitionDuration;
    float elapsedTime;
    float currentAlpha;

    // デフォルト設定
    float defaultDuration;
    glm::vec4 overlayColor;

    // スライド用オフセット
    glm::vec2 slideOffset;

    // コールバック
    std::function<void()> onOutgoingComplete;
    std::function<void()> onIncomingComplete;
    bool outgoingCallbackFired;

    // イージング関数
    float easeInOut(float t) const;
    float easeOutCubic(float t) const;

    // 遷移タイプ別更新
    void updateFade(float progress);
    void updateSlide(float progress);
};
