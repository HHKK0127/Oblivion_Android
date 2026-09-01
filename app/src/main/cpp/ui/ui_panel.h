#pragma once

#include "ui_component.h"
#include <string>
#include <functional>

/**
 * @brief UI panel (window/dialog foundation)
 *
 * Phase 9: Container panel with background, title bar, and border.
 * Supports drag movement, close button, and title text display.
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

    // === Title ===
    void setTitle(const std::string& title);
    const std::string& getTitle() const { return titleText; }
    void setTitleColor(const glm::vec4& color) { titleColor = color; }

    // === Style ===
    void setTitleBarHeight(float height) { titleBarHeight = height; }
    void setTitleBarColor(const glm::vec4& color) { titleBarColor = color; }
    void setCloseButtonVisible(bool visible) { closeButtonVisible = visible; }
    void setDraggable(bool draggable) { isDraggable = draggable; }

    // === Callbacks ===
    using CloseCallback = std::function<void()>;
    void setOnClose(CloseCallback cb) { onCloseCallback = cb; }

    // === Content Margins ===
    void setContentMargin(float margin) { contentMargin = margin; }
    glm::vec2 getContentPosition() const;
    glm::vec2 getContentSize() const;

protected:
    void renderTitleBar() const;
    void renderCloseButton() const;
    bool isInsideTitleBar(float x, float y) const;
    bool isInsideCloseButton(float x, float y) const;

    // Drag state (public to distinguish from title bar drag in derived classes)
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

    // Close button
    float closeButtonSize;
    glm::vec4 closeButtonColor;
    CloseCallback onCloseCallback;
};
