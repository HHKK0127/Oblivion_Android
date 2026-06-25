#include "ui_button.h"
#include "text_renderer.h"
#include "ui_system.h"
#include "../audio/audio_manager.h"
#include <cmath>

UIButton::UIButton(const std::string& name)
    : UIComponent(name),
      label("Button"),
      labelColor(1.0f, 1.0f, 1.0f),
      labelScale(1.0f),
      textRenderer(nullptr),
      enabled(true),
      pressed(false),
      hovered(false),
      normalColor(0.25f, 0.25f, 0.3f, 0.9f),
      pressedColor(0.15f, 0.15f, 0.18f, 1.0f),
      hoverColor(0.35f, 0.35f, 0.4f, 0.95f),
      disabledColor(0.15f, 0.15f, 0.15f, 0.5f),
      pressAnimTimer(0.0f),
      hoverScaleTimer(0.0f) {
}

UIButton::~UIButton() {
    cleanup();
}

bool UIButton::initialize() {
    if (!UIComponent::initialize()) return false;
    updateVisualState();
    return true;
}

void UIButton::update(float deltaTime) {
    UIComponent::update(deltaTime);

    // Press animation decay
    if (pressAnimTimer > 0.0f) {
        pressAnimTimer -= deltaTime;
        if (pressAnimTimer < 0.0f) {
            pressAnimTimer = 0.0f;
            pressed = false;
            updateVisualState();
        }
    }

    // Hover scale animation
    if (hovered && hoverScaleTimer < HOVER_SCALE_DURATION) {
        hoverScaleTimer += deltaTime;
        if (hoverScaleTimer > HOVER_SCALE_DURATION) hoverScaleTimer = HOVER_SCALE_DURATION;
    } else if (!hovered && hoverScaleTimer > 0.0f) {
        hoverScaleTimer -= deltaTime;
        if (hoverScaleTimer < 0.0f) hoverScaleTimer = 0.0f;
    }
}

void UIButton::render() {
    if (!isVisible()) return;

    // Update texture based on state before rendering
    GLuint stateTexture = 0;
    if (!enabled && disabledTexture != 0) {
        stateTexture = disabledTexture;
    } else if (pressed && pressedTexture != 0) {
        stateTexture = pressedTexture;
    } else if (hovered && hoverTexture != 0) {
        stateTexture = hoverTexture;
    } else if (normalTexture != 0) {
        stateTexture = normalTexture;
    }
    if (stateTexture != 0) {
        setTexture(stateTexture);
    }

    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Apply hover scale: expand around center
    glm::vec2 origPos = getPosition();
    glm::vec2 origSize = getSize();
    if (hoverScaleTimer > 0.0f && !pressed) {
        float t = hoverScaleTimer / HOVER_SCALE_DURATION;
        float scale = 1.0f + (HOVER_SCALE_MAX - 1.0f) * t;
        float dw = origSize.x * (scale - 1.0f) * 0.5f;
        float dh = origSize.y * (scale - 1.0f) * 0.5f;
        setPosition(origPos.x - dw, origPos.y - dh);
        setSize(origSize.x * scale, origSize.y * scale);
    }

    // Render button background using updated visual state (includes children)
    UIComponent::render();

    // Render label text
    renderLabel();

    // Restore position/size if scaled
    if (hoverScaleTimer > 0.0f && !pressed) {
        setPosition(origPos.x, origPos.y);
        setSize(origSize.x, origSize.y);
    }

    // Restore state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIButton::cleanup() {
    UIComponent::cleanup();
}

bool UIButton::onEvent(const UIEvent& event) {
    if (!isVisible() || !enabled) return false;

    if (!contains(event.x, event.y)) {
        if (event.type == UIEventType::TOUCH_UP) {
            pressed = false;
            updateVisualState();
        }
        return false;
    }

    switch (event.type) {
        case UIEventType::TOUCH_DOWN:
            pressed = true;
            pressAnimTimer = PRESS_ANIM_DURATION;
            updateVisualState();
            if (onClickCallback) {
                onClickCallback();
            }
            // Play notification sound
            if (g_audioManager) {
                g_audioManager->playSound("ui/notification");
            }
            return true;

        case UIEventType::TOUCH_UP:
            if (pressed) {
                pressed = false;
                updateVisualState();
            }
            return true;

        case UIEventType::TOUCH_MOVE:
            // Keep pressed state while inside
            return true;

        default:
            return true;
    }
}

bool UIButton::onTouchDown(float x, float y, int pointerId) {
    UIEvent event(UIEventType::TOUCH_DOWN, x, y, pointerId);
    return onEvent(event);
}

bool UIButton::onTouchUp(float x, float y, int pointerId) {
    UIEvent event(UIEventType::TOUCH_UP, x, y, pointerId);
    return onEvent(event);
}

void UIButton::setLabel(const std::string& text) {
    label = text;
}

void UIButton::setEnabled(bool e) {
    enabled = e;
    if (!enabled) {
        pressed = false;
    }
    updateVisualState();
}

void UIButton::updateVisualState() {
    if (!enabled) {
        setBackgroundColor(disabledColor);
    } else if (pressed) {
        setBackgroundColor(pressedColor);
    } else if (hovered) {
        setBackgroundColor(hoverColor);
    } else {
        setBackgroundColor(normalColor);
    }
}

void UIButton::renderLabel() const {
    if (!textRenderer || label.empty()) return;

    glm::vec2 absPos = getAbsolutePosition();

    // Estimate text size (rough approximation: ~16px per char at scale 1.0)
    float approxCharWidth = 16.0f * labelScale;
    float textWidth = approxCharWidth * static_cast<float>(label.length());
    float textHeight = 32.0f * labelScale;

    // Center the text within the button
    glm::vec2 btnSize = getSize();
    float textX = absPos.x + (btnSize.x - textWidth) * 0.5f;
    float textY = absPos.y + (btnSize.y - textHeight) * 0.5f;

    // Clamp to integer positions for sharper text
    textX = std::round(textX);
    textY = std::round(textY);

    textRenderer->renderText(label, textX, textY, labelColor, labelScale);
}

glm::vec4 UIButton::getCurrentColor() const {
    if (!enabled) return disabledColor;
    if (pressed) return pressedColor;
    if (hovered) return hoverColor;
    return normalColor;
}
