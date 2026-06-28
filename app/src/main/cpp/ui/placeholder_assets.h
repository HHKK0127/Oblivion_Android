#pragma once

#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <vector>

/**
 * @brief プレースホルダーアセット生成ユーティリティ
 *
 * Phase 9: UI フレームワーク基盤での仮アセット生成機能
 * 羊皮紙/古書風テーマのパレットを使用した
 * プログラム生成プレースホルダーを提供
 */
namespace PlaceholderAssets {

    // ========================================
    // カラーパレット定義（羊皮紙風）
    // ========================================

    /// Oblivion UI テーマカラー定義
    namespace Colors {
        const glm::vec3 PARCHMENT_LIGHT(0.77f, 0.66f, 0.50f);   // #C4A97F - 明るい羊皮紙
        const glm::vec3 PARCHMENT_DARK(0.55f, 0.45f, 0.33f);    // #8B7355 - 暗い羊皮紙
        const glm::vec3 BROWN_ACCENT(0.36f, 0.25f, 0.20f);      // #5C4033 - 茶色アクセント
        const glm::vec3 GOLD_HIGHLIGHT(0.85f, 0.65f, 0.13f);    // #DAA520 - 金色ハイライト
        const glm::vec3 RED_HEALTH(0.80f, 0.10f, 0.10f);        // #CC1919 - HP（赤）
        const glm::vec3 BLUE_MANA(0.20f, 0.40f, 0.80f);         // #3366CC - MP（青）
        const glm::vec3 GREEN_STAMINA(0.40f, 0.70f, 0.30f);     // #66B319 - ST（緑）
        const glm::vec3 DARK_GRAY(0.20f, 0.20f, 0.20f);         // #333333 - 暗いグレー
        const glm::vec3 LIGHT_GRAY(0.80f, 0.80f, 0.80f);        // #CCCCCC - 明るいグレー
    }

    // ========================================
    // 描画ユーティリティ関数
    // ========================================

    /**
     * @brief 単色矩形描画
     * @param x 左端X座標（スクリーン座標）
     * @param y 上端Y座標（スクリーン座標）
     * @param width 幅（ピクセル）
     * @param height 高さ（ピクセル）
     * @param color 描画色（RGB）
     */
    void drawSolidRect(float x, float y, float width, float height, const glm::vec3& color);

    /**
     * @brief 単色矩形描画（透明度付き）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     * @param color 描画色（RGB）
     * @param alpha 透明度（0.0-1.0）
     */
    void drawSolidRect(float x, float y, float width, float height,
                      const glm::vec3& color, float alpha);

    /**
     * @brief 縁取り付きパネル描画（羊皮紙風）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     */
    void drawPanel(float x, float y, float width, float height);

    /**
     * @brief パネル描画（カスタムカラー）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     * @param bgColor 背景色
     * @param borderColor 枠色
     */
    void drawPanel(float x, float y, float width, float height,
                  const glm::vec3& bgColor, const glm::vec3& borderColor);

    /**
     * @brief ステータスバー描画（背景+フィル）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     * @param fillRatio 充填比率（0.0-1.0）
     * @param fillColor フィル色
     */
    void drawStatusBar(float x, float y, float width, float height,
                      float fillRatio, const glm::vec3& fillColor);

    /**
     * @brief HP バー描画（赤色）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     * @param fillRatio 充填比率（0.0-1.0）
     */
    inline void drawHPBar(float x, float y, float width, float height, float fillRatio) {
        drawStatusBar(x, y, width, height, fillRatio, Colors::RED_HEALTH);
    }

    /**
     * @brief MP バー描画（青色）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     * @param fillRatio 充填比率（0.0-1.0）
     */
    inline void drawMPBar(float x, float y, float width, float height, float fillRatio) {
        drawStatusBar(x, y, width, height, fillRatio, Colors::BLUE_MANA);
    }

    /**
     * @brief スタミナバー描画（緑色）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param width 幅
     * @param height 高さ
     * @param fillRatio 充填比率（0.0-1.0）
     */
    inline void drawStaminaBar(float x, float y, float width, float height, float fillRatio) {
        drawStatusBar(x, y, width, height, fillRatio, Colors::GREEN_STAMINA);
    }

    /**
     * @brief アイコン枠描画
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param size サイズ（正方形）
     */
    void drawIconFrame(float x, float y, float size);

    /**
     * @brief チェックボックス描画（未チェック）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param size サイズ（正方形）
     */
    void drawCheckboxOff(float x, float y, float size);

    /**
     * @brief チェックボックス描画（チェック済み）
     * @param x 左端X座標
     * @param y 上端Y座標
     * @param size サイズ（正方形）
     */
    void drawCheckboxOn(float x, float y, float size);

    /**
     * @brief 円形マーカー描画
     * @param x 中心X座標
     * @param y 中心Y座標
     * @param radius 半径
     * @param color 描画色
     */
    void drawCircleMarker(float x, float y, float radius, const glm::vec3& color);

    /**
     * @brief 三角形マーカー描画（上向き）
     * @param x 中心X座標
     * @param y 中心Y座標
     * @param size サイズ
     * @param color 描画色
     */
    void drawTriangleMarker(float x, float y, float size, const glm::vec3& color);

    // ========================================
    // 初期化・クリーンアップ
    // ========================================

    /**
     * @brief プレースホルダーアセットシステムを初期化
     * @return 成功時true
     */
    bool initialize();

    /**
     * @brief プレースホルダーアセットをクリーンアップ
     */
    void cleanup();

} // namespace PlaceholderAssets
