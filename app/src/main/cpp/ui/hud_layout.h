#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <android/log.h>

#define HUD_LAYOUT_LOG_TAG "HUDLayout"
#define HLAYOUT_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, HUD_LAYOUT_LOG_TAG, __VA_ARGS__)
#define HLAYOUT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, HUD_LAYOUT_LOG_TAG, __VA_ARGS__)

class TextRenderer;

/**
 * @brief HUD要素タイプ
 */
enum class HUDElementType {
    HEALTH_BAR,
    MANA_BAR,
    STAMINA_BAR,
    QUICK_SLOTS,
    COMPASS,
    MINIMAP,
    TARGET_INFO,
    PLAYER_LEVEL,
    ACTIVE_EFFECTS,
    ACTION_PROMPT
};

/**
 * @brief HUD要素配置情報
 */
struct HUDElementLayout {
    HUDElementType type;
    glm::vec2 position;   // ピクセル座標
    glm::vec2 size;       // ピクセルサイズ
    bool visible;
    float scale;          // スケーリング倍率

    HUDElementLayout()
        : type(HUDElementType::HEALTH_BAR), position(0.0f), size(0.0f),
          visible(true), scale(1.0f) {}
};

/**
 * @brief HUDレイアウト管理
 *
 * Phase 43: HUD要素の配置と画面サイズへの自動適応を管理する。
 * - ヘルスバー（左上）
 * - マナバー（右上）
 * - スタミナバー（中央下）
 * - クイックスロット（右下）
 * - コンパス（上部中央）
 * - ミニマップ（左上、ヘルスバー下）
 * - ターゲット情報（中央）
 */
class HUDLayout {
public:
    HUDLayout();
    ~HUDLayout() = default;

    /**
     * @brief 初期化
     * @param screenWidth スクリーン幅
     * @param screenHeight スクリーン高さ
     */
    void initialize(int screenWidth, int screenHeight);

    /**
     * @brief スクリーンサイズ変更時にレイアウトを再計算
     */
    void recalculate(int screenWidth, int screenHeight);

    /**
     * @brief HUD要素のレイアウト情報を取得
     * @param type 要素タイプ
     * @return レイアウト情報
     */
    const HUDElementLayout& getElementLayout(HUDElementType type) const;

    /**
     * @brief HUD要素の位置をオーバーライド
     */
    void setElementPosition(HUDElementType type, const glm::vec2& position);

    /**
     * @brief HUD要素のサイズをオーバーライド
     */
    void setElementSize(HUDElementType type, const glm::vec2& size);

    /**
     * @brief HUD要素の可視性を設定
     */
    void setElementVisible(HUDElementType type, bool visible);

    /**
     * @brief HUD要素のスケールを設定
     */
    void setElementScale(HUDElementType type, float scale);

    /**
     * @brief 全HUD要素のリストを取得
     */
    const std::vector<HUDElementLayout>& getAllElements() const { return elements; }

    /**
     * @brief UIスケールファクターを取得（DPI対応）
     */
    float getUIScale() const { return uiScale; }

    /**
     * @brief マージンを設定
     */
    void setMargin(float margin) { edgeMargin = margin; recalculate(screenWidth, screenHeight); }

    /**
     * @brief バーの高さを設定
     */
    void setBarHeight(float height) { barHeight = height; recalculate(screenWidth, screenHeight); }

    /**
     * @brief デバッグ用: 全要素の位置をログ出力
     */
    void debugLogLayout() const;

private:
    int screenWidth;
    int screenHeight;
    float uiScale;
    float edgeMargin;
    float barHeight;
    float barWidth;
    float quickSlotSize;
    float minimapSize;
    float compassHeight;

    // 全要素のレイアウト
    std::vector<HUDElementLayout> elements;

    // 要素検索ヘルパー
    HUDElementLayout* findElement(HUDElementType type);
    const HUDElementLayout* findElement(HUDElementType type) const;

    // デフォルトレイアウト計算
    void calculateDefaultLayout();
};
