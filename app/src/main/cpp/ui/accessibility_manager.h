#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <android/log.h>

#define ACCESS_LOG_TAG "AccessibilityManager"
#define ACCESS_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, ACCESS_LOG_TAG, __VA_ARGS__)
#define ACCESS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, ACCESS_LOG_TAG, __VA_ARGS__)

/**
 * @brief 色覚タイプ
 */
enum class ColorBlindMode {
    NONE,
    PROTANOPIA,    // 赤色盲
    DEUTERANOPIA,  // 緑色盲
    TRITANOPIA     // 青色盲
};

/**
 * @brief アクセシビリティ設定変更コールバック
 */
using AccessibilityChangeCallback = std::function<void()>;

/**
 * @brief アクセシビリティ管理
 *
 * Phase 43: ゲームのアクセシビリティ設定を管理する。
 * - テキストサイズ調整
 * - コントラスト調整
 * - 操作感度調整
 * - 色覚サポート
 */
class AccessibilityManager {
public:
    AccessibilityManager();
    ~AccessibilityManager() = default;

    /**
     * @brief 初期化
     */
    void initialize();

    // === テキストサイズ ===

    /**
     * @brief テキストスケールファクターを設定（0.5〜2.0）
     */
    void setTextScale(float scale);

    /**
     * @brief テキストスケールファクターを取得
     */
    float getTextScale() const { return textScale; }

    // === コントラスト ===

    /**
     * @brief コントラスト倍率を設定（0.5〜2.0、1.0が標準）
     */
    void setContrastMultiplier(float multiplier);

    /**
     * @brief コントラスト倍率を取得
     */
    float getContrastMultiplier() const { return contrastMultiplier; }

    /**
     * @brief 高コントラストモードを切替
     */
    void setHighContrastMode(bool enabled);

    /**
     * @brief 高コントラストモードか
     */
    bool isHighContrastMode() const { return highContrastMode; }

    // === 操作感度 ===

    /**
     * @brief タッチ感度を設定（0.5〜2.0、1.0が標準）
     */
    void setTouchSensitivity(float sensitivity);

    /**
     * @brief タッチ感度を取得
     */
    float getTouchSensitivity() const { return touchSensitivity; }

    /**
     * @brief カメラ感度を設定（0.5〜2.0）
     */
    void setCameraSensitivity(float sensitivity);

    /**
     * @brief カメラ感度を取得
     */
    float getCameraSensitivity() const { return cameraSensitivity; }

    // === 色覚サポート ===

    /**
     * @brief 色覚モードを設定
     */
    void setColorBlindMode(ColorBlindMode mode);

    /**
     * @brief 色覚モードを取得
     */
    ColorBlindMode getColorBlindMode() const { return colorBlindMode; }

    /**
     * @brief 色を色覚モードに応じて変換
     * @param color 元の色
     * @return 変換後の色
     */
    glm::vec4 applyColorBlindFilter(const glm::vec4& color) const;

    // === コールバック ===

    /**
     * @brief 設定変更コールバックを登録
     */
    void registerChangeCallback(AccessibilityChangeCallback callback);

    // === リセット ===

    /**
     * @brief 全設定をデフォルトに戻す
     */
    void resetToDefaults();

private:
    float textScale;
    float contrastMultiplier;
    float touchSensitivity;
    float cameraSensitivity;
    bool highContrastMode;
    ColorBlindMode colorBlindMode;

    std::vector<AccessibilityChangeCallback> changeCallbacks;

    void notifyChange();

    // 色覚変換行列
    glm::vec4 applyProtanopia(const glm::vec4& color) const;
    glm::vec4 applyDeuteranopia(const glm::vec4& color) const;
    glm::vec4 applyTritanopia(const glm::vec4& color) const;
};
