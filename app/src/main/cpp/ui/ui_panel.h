#pragma once

#include "ui_component.h"
#include <string>
#include <functional>

/**
 * @brief UIパネル（ウィンドウ/ダイアログの基盤）
 *
 * Phase 9: 背景、タイトルバー、ボーダーを持つコンテナパネル。
 * ドラッグ移動、閉じるボタン、タイトルテキスト表示に対応。
 */
class UIPanel : public UIComponent {
public:
    UIPanel(const std::string& name = "UIPanel");
    ~UIPanel() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;

    bool onEvent(const UIEvent& event) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    bool onTouchUp(float x, float y, int pointerId) override;
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;

    // === タイトル ===
    void setTitle(const std::string& title);
    const std::string& getTitle() const { return titleText; }
    void setTitleColor(const glm::vec4& color) { titleColor = color; }

    // === スタイル ===
    void setTitleBarHeight(float height) { titleBarHeight = height; }
    void setTitleBarColor(const glm::vec4& color) { titleBarColor = color; }
    void setCloseButtonVisible(bool visible) { closeButtonVisible = visible; }
    void setDraggable(bool draggable) { isDraggable = draggable; }

    // === コールバック ===
    using CloseCallback = std::function<void()>;
    void setOnClose(CloseCallback cb) { onCloseCallback = cb; }

    // === 内容物のマージン ===
    void setContentMargin(float margin) { contentMargin = margin; }
    glm::vec2 getContentPosition() const;
    glm::vec2 getContentSize() const;

protected:
    void renderTitleBar() const;
    void renderCloseButton() const;
    bool isInsideTitleBar(float x, float y) const;
    bool isInsideCloseButton(float x, float y) const;

    // ドラッグ状態（派生クラスでタイトルバードラッグと区別するために公開）
    bool isDragging;
    glm::vec2 dragOffset;

private:
    std::string titleText;
    glm::vec4 titleColor;
    float titleBarHeight;
    glm::vec4 titleBarColor;
    bool closeButtonVisible;
    bool isDraggable;
    float contentMargin;

    // 閉じるボタン
    float closeButtonSize;
    glm::vec4 closeButtonColor;
    CloseCallback onCloseCallback;
};
