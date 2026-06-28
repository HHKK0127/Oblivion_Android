#include "ui_toast.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

UIToast::UIToast() = default;

bool UIToast::initialize(TextRenderer* tr, int screenW, int screenH) {
    if (!tr) return false;
    textRenderer = tr;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIToast::showToast(const Toast& toast) {
    toastQueue_.push(toast);
    if (currentToast_ == nullptr && !toastQueue_.empty()) {
        displayNextToast();
    }
}

void UIToast::displayNextToast() {
    if (toastQueue_.empty()) {
        currentToast_ = nullptr;
        return;
    }

    // Move the first toast from queue to current
    static Toast current("", NOTIFICATION, 0.0f);  // Static to persist
    current = toastQueue_.front();
    toastQueue_.pop();
    currentToast_ = &current;

    elapsedTime_ = 0.0f;
    fadeAlpha_ = 1.0f;
}

void UIToast::dismissCurrentToast() {
    displayNextToast();
}

void UIToast::update(float deltaTime) {
    if (!currentToast_) return;

    elapsedTime_ += deltaTime;

    // Fade out in the last 0.5 seconds
    float fadeStartTime = currentToast_->duration - 0.5f;
    if (elapsedTime_ >= fadeStartTime) {
        fadeAlpha_ = std::max(0.0f, (currentToast_->duration - elapsedTime_) / 0.5f);
    }

    // Dismiss when time is up
    if (elapsedTime_ >= currentToast_->duration) {
        dismissCurrentToast();
    }
}

void UIToast::render() {
    if (!currentToast_ || !textRenderer || fadeAlpha_ <= 0.0f) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float toastX = (screenWidth - 400.0f) / 2.0f;
    float toastY = getToastY();
    float toastW = 400.0f;
    float toastH = 48.0f;

    // Toast background with alpha fade
    glm::vec4 bgColor(getToastBackgroundColor().x, getToastBackgroundColor().y, getToastBackgroundColor().z, 0.85f * fadeAlpha_);
    UIDrawHelper::drawColoredQuad(toastX, toastY, toastW, toastH, bgColor,
        screenWidth, screenHeight);

    // Toast border
    glm::vec4 borderColor(getToastColor().x, getToastColor().y, getToastColor().z, 1.0f * fadeAlpha_);
    UIDrawHelper::drawBorder(toastX, toastY, toastW, toastH, 2.0f, borderColor,
        screenWidth, screenHeight);

    // Toast text
    glm::vec3 textColor = getToastColor();
    textRenderer->renderText(currentToast_->message,
        toastX + 15.0f, toastY + 12.0f,
        textColor, 0.7f);

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

glm::vec3 UIToast::getToastColor() const {
    if (!currentToast_) return glm::vec3(1.0f, 1.0f, 1.0f);

    switch (currentToast_->type) {
    case ITEM_ACQUIRED:
        return PlaceholderAssets::Colors::GOLD_HIGHLIGHT;
    case QUEST_UPDATED:
        return glm::vec3(0.3f, 0.5f, 0.9f);  // Blue
    case LEVEL_UP:
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green
    case NOTIFICATION:
        return PlaceholderAssets::Colors::PARCHMENT_LIGHT;
    }
    return glm::vec3(1.0f, 1.0f, 1.0f);
}

glm::vec3 UIToast::getToastBackgroundColor() const {
    if (!currentToast_) return glm::vec3(0.2f, 0.2f, 0.2f);

    switch (currentToast_->type) {
    case ITEM_ACQUIRED:
        return glm::vec3(0.3f, 0.25f, 0.1f);  // Gold-ish
    case QUEST_UPDATED:
        return glm::vec3(0.15f, 0.2f, 0.35f);  // Dark blue
    case LEVEL_UP:
        return glm::vec3(0.1f, 0.3f, 0.1f);    // Dark green
    case NOTIFICATION:
        return glm::vec3(0.15f, 0.12f, 0.08f);  // Dark parchment
    }
    return glm::vec3(0.2f, 0.2f, 0.2f);
}

float UIToast::getToastY() const {
    // Display at top of screen with small margin
    return 20.0f;
}
