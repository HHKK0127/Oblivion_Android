#include "ui_panel.h"
#include "ui_draw_helper.h"
#include <cmath>

UIPanel::UIPanel(const std::string& name)
    : UIComponent(name),
      titleText("Panel"),
      titleColor(1.0f, 1.0f, 1.0f, 1.0f),
      titleBarHeight(40.0f),
      titleBarColor(0.15f, 0.15f, 0.2f, 0.95f),
      closeButtonVisible(true),
      isDraggable(true),
      contentMargin(10.0f),
      isDragging(false),
      dragOffset(0.0f, 0.0f),
      closeButtonSize(24.0f),
      closeButtonColor(0.8f, 0.2f, 0.2f, 1.0f) {
}

UIPanel::~UIPanel() {
    cleanup();
}

bool UIPanel::initialize() {
    if (!UIComponent::initialize()) return false;

    // Default panel background
    setBackgroundColor(glm::vec4(0.1f, 0.1f, 0.12f, 0.9f));
    setBorderColor(glm::vec4(0.3f, 0.3f, 0.35f, 1.0f));
    setBorderWidth(2.0f);

    return true;
}

void UIPanel::update(float deltaTime) {
    UIComponent::update(deltaTime);
}

void UIPanel::render() {
    if (!isVisible()) return;

    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render base panel (background, texture, border, children)
    UIComponent::render();

    // Render title bar and close button
    renderTitleBar();
    if (closeButtonVisible) {
        renderCloseButton();
    }

    // Restore state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIPanel::cleanup() {
    UIComponent::cleanup();
}

bool UIPanel::onEvent(const UIEvent& event) {
    if (!isVisible() || !isEnabled()) return false;

    // Check close button first
    if (closeButtonVisible && event.type == UIEventType::TOUCH_DOWN) {
        if (isInsideCloseButton(event.x, event.y)) {
            if (onCloseCallback) {
                onCloseCallback();
            }
            hide();
            return true;
        }
    }

    // Then try children
    if (dispatchEventToChildren(event)) return true;

    // Handle drag on title bar
    if (event.type == UIEventType::TOUCH_DOWN && isInsideTitleBar(event.x, event.y)) {
        if (isDraggable) {
            isDragging = true;
            glm::vec2 absPos = getAbsolutePosition();
            dragOffset = glm::vec2(event.x - absPos.x, event.y - absPos.y);
            return true;
        }
    }

    // Panel body touch
    if (contains(event.x, event.y)) {
        return true;
    }

    return false;
}

bool UIPanel::onTouchDown(float x, float y, int pointerId) {
    UIEvent event(UIEventType::TOUCH_DOWN, x, y, pointerId);
    return onEvent(event);
}

bool UIPanel::onTouchUp(float x, float y, int pointerId) {
    isDragging = false;
    UIEvent event(UIEventType::TOUCH_UP, x, y, pointerId);
    return onEvent(event);
}

bool UIPanel::onTouchMove(float x, float y, float dx, float dy, int pointerId) {
    if (isDragging && isDraggable) {
        // Update position based on drag
        glm::vec2 newPos(x - dragOffset.x, y - dragOffset.y);

        // Simple clamp to screen bounds (consider parent if any)
        newPos.x = std::max(0.0f, std::min(newPos.x, static_cast<float>(screenWidth) - size.x));
        newPos.y = std::max(0.0f, std::min(newPos.y, static_cast<float>(screenHeight) - size.y));

        setPosition(newPos.x, newPos.y);
        return true;
    }

    UIEvent event(UIEventType::TOUCH_MOVE, x, y, pointerId);
    event.dx = dx;
    event.dy = dy;
    return onEvent(event);
}

void UIPanel::setTitle(const std::string& title) {
    titleText = title;
}

glm::vec2 UIPanel::getContentPosition() const {
    glm::vec2 absPos = getAbsolutePosition();
    return glm::vec2(absPos.x + contentMargin, absPos.y + titleBarHeight + contentMargin);
}

glm::vec2 UIPanel::getContentSize() const {
    return glm::vec2(size.x - contentMargin * 2.0f,
                     size.y - titleBarHeight - contentMargin * 2.0f);
}

void UIPanel::renderTitleBar() const {
    if (titleBarHeight <= 0.0f) return;
    glm::vec2 absPos = getAbsolutePosition();
    UIDrawHelper::drawColoredQuad(absPos.x, absPos.y, size.x, titleBarHeight,
                                  titleBarColor, screenWidth, screenHeight);
}

void UIPanel::renderCloseButton() const {
    if (!closeButtonVisible) return;

    glm::vec2 absPos = getAbsolutePosition();
    float btnX = absPos.x + size.x - closeButtonSize - 8.0f;
    float btnY = absPos.y + (titleBarHeight - closeButtonSize) * 0.5f;

    // Draw close button background
    UIDrawHelper::drawColoredQuad(btnX, btnY, closeButtonSize, closeButtonSize,
                                  closeButtonColor, screenWidth, screenHeight);

    // Draw X mark (simplified as cross)
    glm::vec4 xColor(1.0f, 1.0f, 1.0f, 1.0f);
    float pad = closeButtonSize * 0.25f;
    float thickness = 2.0f;

    // Horizontal line
    UIDrawHelper::drawColoredQuad(btnX + pad, btnY + closeButtonSize * 0.5f - thickness * 0.5f,
                                  closeButtonSize - pad * 2.0f, thickness,
                                  xColor, screenWidth, screenHeight);
    // Vertical line
    UIDrawHelper::drawColoredQuad(btnX + closeButtonSize * 0.5f - thickness * 0.5f, btnY + pad,
                                  thickness, closeButtonSize - pad * 2.0f,
                                  xColor, screenWidth, screenHeight);
}

bool UIPanel::isInsideTitleBar(float x, float y) const {
    glm::vec2 absPos = getAbsolutePosition();
    return (x >= absPos.x && x <= absPos.x + size.x &&
            y >= absPos.y && y <= absPos.y + titleBarHeight);
}

bool UIPanel::isInsideCloseButton(float x, float y) const {
    if (!closeButtonVisible) return false;
    glm::vec2 absPos = getAbsolutePosition();
    float btnX = absPos.x + size.x - closeButtonSize - 8.0f;
    float btnY = absPos.y + (titleBarHeight - closeButtonSize) * 0.5f;
    return (x >= btnX && x <= btnX + closeButtonSize &&
            y >= btnY && y <= btnY + closeButtonSize);
}
