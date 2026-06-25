#pragma once

#include "ui_component.h"
#include <string>
#include <functional>
#include <chrono>

// Forward declaration
class TextRenderer;
class UISystem;

/**
 * @brief UIボタン
 *
 * Phase 9: クリック/タップ可能なボタンコンポーネント。
 * テキストラベル、テクスチャ背景、ホバー/押下/無効状態の
 * ビジュアルフィードバックに対応。
 */
class UIButton : public UIComponent {
public:
    UIButton(const std::string& name = "UIButton");
    ~UIButton() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;

    bool onEvent(const UIEvent& event) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    bool onTouchUp(float x, float y, int pointerId) override;

    // === ラベル ===
    void setLabel(const std::string& text);
    const std::string& getLabel() const { return label; }
    void setLabelColor(const glm::vec3& color) { labelColor = color; }
    void setLabelScale(float scale) { labelScale = scale; }

    // === 状態 ===
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }
    bool isPressed() const { return pressed; }
    bool isHovered() const { return hovered; }

    // === 色設定 ===
    void setNormalColor(const glm::vec4& color) { normalColor = color; }
    void setPressedColor(const glm::vec4& color) { pressedColor = color; }
    void setHoverColor(const glm::vec4& color) { hoverColor = color; }
    void setDisabledColor(const glm::vec4& color) { disabledColor = color; }

    // === コールバック ===
    using ClickCallback = std::function<void()>;
    void setOnClick(ClickCallback cb) { onClickCallback = cb; }

    // === テキストレンダラー ===
    void setTextRenderer(TextRenderer* renderer) { textRenderer = renderer; }

    // === テクスチャ ===
    void setNormalTexture(GLuint texId) { normalTexture = texId; }
    void setHoverTexture(GLuint texId) { hoverTexture = texId; }
    void setPressedTexture(GLuint texId) { pressedTexture = texId; }
    void setDisabledTexture(GLuint texId) { disabledTexture = texId; }

protected:
    void updateVisualState();
    void renderLabel() const;
    glm::vec4 getCurrentColor() const;

private:
    std::string label;
    glm::vec3 labelColor;
    float labelScale;
    TextRenderer* textRenderer;

    // 状態
    bool enabled;
    bool pressed;
    bool hovered;

    // 色
    glm::vec4 normalColor;
    glm::vec4 pressedColor;
    glm::vec4 hoverColor;
    glm::vec4 disabledColor;

    // コールバック
    ClickCallback onClickCallback;

    // アニメーション用タイマー
    float pressAnimTimer;
    float hoverScaleTimer;  // ホバー時スケールアニメーション
    static constexpr float PRESS_ANIM_DURATION = 0.1f;
    static constexpr float HOVER_SCALE_DURATION = 0.1f;
    static constexpr float HOVER_SCALE_MAX = 1.05f;

    // テクスチャ（各状態）
    GLuint normalTexture = 0;
    GLuint hoverTexture = 0;
    GLuint pressedTexture = 0;
    GLuint disabledTexture = 0;
};
