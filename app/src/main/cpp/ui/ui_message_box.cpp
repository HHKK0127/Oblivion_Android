#include "ui_message_box.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UIMessageBox::UIMessageBox(const std::string& title)
    : UIPanel(title.empty() ? "Message" : title) {
    // Gold/cream parchment style for important messages
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.x * 0.95f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.y * 0.9f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.z * 0.8f, 0.98f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.x,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.y,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.z, 1.0f));
    setBorderWidth(3.0f);
    setTitleBarColor(glm::vec4(0.2f, 0.15f, 0.1f, 0.85f));
    setCloseButtonVisible(false);
    setDraggable(false);
}

bool UIMessageBox::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    return UIPanel::initialize();
}

void UIMessageBox::queueMessage(const Message& msg) {
    messageQueue_.push(msg);
    if (currentMessage_ == nullptr && !messageQueue_.empty()) {
        displayNextMessage();
    }
}

void UIMessageBox::displayNextMessage() {
    if (messageQueue_.empty()) {
        currentMessage_ = nullptr;
        setVisible(false);
        return;
    }

    // Move the first message from queue to current
    static Message current("", "");  // Static to persist
    current = messageQueue_.front();
    messageQueue_.pop();
    currentMessage_ = &current;

    setVisible(true);
}

void UIMessageBox::dismissCurrentMessage(bool confirmed) {
    if (currentMessage_ && currentMessage_->callback) {
        currentMessage_->callback(confirmed);
    }
    displayNextMessage();
}

void UIMessageBox::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UIMessageBox::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;
    if (!currentMessage_) return false;

    int buttonIdx = hitTestButton(x, y);
    if (buttonIdx == 0) {  // First button (OK or Yes)
        dismissCurrentMessage(true);
        return true;
    }
    if (buttonIdx == 1) {  // Second button (No or Cancel)
        dismissCurrentMessage(false);
        return true;
    }

    return true;
}

void UIMessageBox::render() {
    if (!isVisible() || !currentMessage_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderTypeIndicator();
    renderMessageDialog();
    renderButtons();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIMessageBox::renderTypeIndicator() {
    if (!textRenderer || !currentMessage_) return;

    glm::vec2 cp = getContentPosition();
    float indicatorW = 6.0f;
    float indicatorH = getDialogHeight() - 20.0f;

    // Left color bar indicating message type
    glm::vec3 typeColor = getTypeColor();
    UIDrawHelper::drawColoredQuad(cp.x + 8.0f, cp.y + 10.0f, indicatorW, indicatorH,
        glm::vec4(typeColor.x, typeColor.y, typeColor.z, 1.0f),
        screenWidth, screenHeight);
}

void UIMessageBox::renderMessageDialog() {
    if (!textRenderer || !currentMessage_) return;

    glm::vec2 cp = getContentPosition();
    float dx = getDialogX();
    float dy = getDialogY();
    float dw = getDialogWidth();

    // Title
    float ty = dy + 10.0f;
    textRenderer->renderText(currentMessage_->title,
        dx + 20.0f, ty,
        glm::vec3(0.0f, 0.0f, 0.0f), 0.8f);

    // Message text (word-wrapped)
    float my = ty + 35.0f;
    textRenderer->renderText(currentMessage_->text,
        dx + 20.0f, my,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.65f);
}

void UIMessageBox::renderButtons() {
    if (!textRenderer || !currentMessage_) return;

    glm::vec2 cp = getContentPosition();
    float by = getButtonY();
    float buttonW = 90.0f;
    float buttonH = 34.0f;
    float spacing = 20.0f;

    // Calculate button positions based on type
    float button1X, button2X;
    const char* button1Text = "OK";
    const char* button2Text = "Cancel";

    switch (currentMessage_->buttons) {
    case OK:
        button1X = getDialogX() + getDialogWidth() / 2.0f - buttonW / 2.0f;
        button1Text = "OK";
        button2Text = nullptr;
        break;
    case YES_NO:
        button1X = getDialogX() + getDialogWidth() / 2.0f - buttonW - spacing / 2.0f;
        button2X = getDialogX() + getDialogWidth() / 2.0f + spacing / 2.0f;
        button1Text = "Yes";
        button2Text = "No";
        break;
    case OK_CANCEL:
        button1X = getDialogX() + getDialogWidth() / 2.0f - buttonW - spacing / 2.0f;
        button2X = getDialogX() + getDialogWidth() / 2.0f + spacing / 2.0f;
        button1Text = "OK";
        button2Text = "Cancel";
        break;
    }

    // Draw first button
    PlaceholderAssets::drawPanel(button1X, by, buttonW, buttonH,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    textRenderer->renderText(button1Text,
        button1X + 15.0f, by + 6.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);

    // Draw second button if needed
    if (button2Text != nullptr) {
        PlaceholderAssets::drawPanel(button2X, by, buttonW, buttonH,
            PlaceholderAssets::Colors::BROWN_ACCENT,
            glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
        textRenderer->renderText(button2Text,
            button2X + 10.0f, by + 6.0f,
            glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
    }
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UIMessageBox::hitTestButton(float x, float y) const {
    if (!currentMessage_) return -1;

    float by = getButtonY();
    float buttonW = 90.0f;
    float buttonH = 34.0f;
    float spacing = 20.0f;

    float button1X;
    switch (currentMessage_->buttons) {
    case OK:
        button1X = getDialogX() + getDialogWidth() / 2.0f - buttonW / 2.0f;
        if (x >= button1X && x <= button1X + buttonW &&
            y >= by && y <= by + buttonH) {
            return 0;
        }
        break;
    case YES_NO:
    case OK_CANCEL:
        button1X = getDialogX() + getDialogWidth() / 2.0f - buttonW - spacing / 2.0f;
        float button2X = getDialogX() + getDialogWidth() / 2.0f + spacing / 2.0f;

        if (x >= button1X && x <= button1X + buttonW &&
            y >= by && y <= by + buttonH) {
            return 0;
        }
        if (x >= button2X && x <= button2X + buttonW &&
            y >= by && y <= by + buttonH) {
            return 1;
        }
        break;
    }

    return -1;
}

// ─── layout helpers ───────────────────────────────────────────────────────────

float UIMessageBox::getDialogWidth() const {
    return std::min(600.0f, screenWidth - 100.0f);
}

float UIMessageBox::getDialogHeight() const {
    return 240.0f;
}

float UIMessageBox::getDialogX() const {
    return (screenWidth - getDialogWidth()) / 2.0f;
}

float UIMessageBox::getDialogY() const {
    return (screenHeight - getDialogHeight()) / 2.0f;
}

float UIMessageBox::getButtonY() const {
    return getDialogY() + getDialogHeight() - 45.0f;
}

glm::vec3 UIMessageBox::getTypeColor() const {
    if (!currentMessage_) return glm::vec3(0.5f, 0.5f, 0.5f);

    switch (currentMessage_->type) {
    case INFO:
        return glm::vec3(0.2f, 0.4f, 0.8f);  // Blue
    case SUCCESS:
        return glm::vec3(0.2f, 0.7f, 0.2f);  // Green
    case WARNING:
        return glm::vec3(0.8f, 0.7f, 0.2f);  // Yellow
    case ERROR:
        return glm::vec3(0.8f, 0.2f, 0.2f);  // Red
    }
    return glm::vec3(0.5f, 0.5f, 0.5f);
}
