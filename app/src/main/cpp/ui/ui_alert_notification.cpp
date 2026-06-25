#include "ui_alert_notification.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

UIAlertNotification::UIAlertNotification() = default;

bool UIAlertNotification::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer_ = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIAlertNotification::addAlert(const std::string& message, AlertType type,
                                    AlertPriority priority, float duration) {
    auto alert = std::make_shared<Alert>();
    alert->message = message;
    alert->type = type;
    alert->priority = priority;
    alert->duration = duration;
    alert->elapsedTime = 0.0f;
    alert->color = getAlertColor(type, priority);
    alert->isActive = true;

    // If no current alert, set this as current
    if (!currentAlert_ || !currentAlert_->isActive) {
        currentAlert_ = alert;
    } else {
        // Queue it if at max capacity
        if (alertQueue_.size() < MAX_QUEUE_SIZE) {
            alertQueue_.push_back(alert);
        }
    }
}

void UIAlertNotification::dismissAlert() {
    if (currentAlert_) {
        currentAlert_->isActive = false;
    }
    processQueue();
}

void UIAlertNotification::clearAllAlerts() {
    currentAlert_.reset();
    alertQueue_.clear();
}

void UIAlertNotification::update(float deltaTime) {
    if (currentAlert_ && currentAlert_->isActive) {
        currentAlert_->elapsedTime += deltaTime;
        if (currentAlert_->elapsedTime >= currentAlert_->duration) {
            dismissAlert();
        }
    }
}

void UIAlertNotification::render() {
    if (!hasActiveAlert() || !textRenderer_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderAlert();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIAlertNotification::renderAlert() {
    float alertX = ALERT_X - ALERT_WIDTH / 2.0f;
    float alertY = ALERT_Y;
    float alpha = getCurrentAlpha();

    // Alert background
    glm::vec4 bgColor(currentAlert_->color.x * 0.3f, currentAlert_->color.y * 0.3f,
                     currentAlert_->color.z * 0.3f, 0.75f * alpha);
    UIDrawHelper::drawColoredQuad(alertX, alertY, ALERT_WIDTH, ALERT_HEIGHT,
        bgColor, screenWidth, screenHeight);

    // Alert border
    glm::vec4 borderColor(currentAlert_->color, alpha);
    UIDrawHelper::drawBorder(alertX, alertY, ALERT_WIDTH, ALERT_HEIGHT, 2.0f,
        borderColor, screenWidth, screenHeight);

    // Icon
    renderAlertIcon();

    // Text
    renderAlertText();
}

void UIAlertNotification::renderAlertIcon() {
    float iconX = ALERT_X - ALERT_WIDTH / 2.0f + 12.0f;
    float iconY = ALERT_Y + 15.0f;
    float alpha = getCurrentAlpha();

    std::string icon = getAlertIcon(currentAlert_->type);
    glm::vec3 iconColor = currentAlert_->color * alpha;

    textRenderer_->renderText(icon,
        iconX, iconY,
        iconColor, getAlertScale(currentAlert_->priority));
}

void UIAlertNotification::renderAlertText() {
    float textX = ALERT_X - ALERT_WIDTH / 2.0f + 40.0f;
    float textY = ALERT_Y + 10.0f;
    float alpha = getCurrentAlpha();

    glm::vec3 textColor = currentAlert_->color * alpha;

    // Main alert message
    textRenderer_->renderText(currentAlert_->message,
        textX, textY,
        textColor, 0.8f);

    // Priority indicator
    std::string priorityLabel = "";
    switch (currentAlert_->priority) {
    case PRIORITY_CRITICAL:
        priorityLabel = "CRITICAL";
        break;
    case PRIORITY_HIGH:
        priorityLabel = "HIGH";
        break;
    case PRIORITY_NORMAL:
        priorityLabel = "ALERT";
        break;
    default:
        break;
    }

    if (!priorityLabel.empty()) {
        textRenderer_->renderText(priorityLabel,
            textX, textY + 18.0f,
            glm::vec3(0.8f, 0.8f, 0.2f) * alpha, 0.6f);
    }
}

glm::vec3 UIAlertNotification::getAlertColor(AlertType type, AlertPriority priority) const {
    switch (type) {
    case ALERT_DETECTED:
        return glm::vec3(1.0f, 0.8f, 0.2f);  // Gold
    case ALERT_DANGER:
        return glm::vec3(1.0f, 0.4f, 0.2f);  // Orange
    case ALERT_WARNING:
        return glm::vec3(1.0f, 0.6f, 0.2f);  // Orange-gold
    case ALERT_CRITICAL:
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    case ALERT_STEALTH:
        return glm::vec3(0.3f, 0.8f, 0.3f);  // Green
    case ALERT_SPELL:
        return glm::vec3(0.5f, 0.7f, 1.0f);  // Light blue
    case ALERT_EFFECT:
        return glm::vec3(0.8f, 0.5f, 1.0f);  // Purple
    default:
        return glm::vec3(0.8f, 0.8f, 0.8f);  // Gray
    }
}

std::string UIAlertNotification::getAlertIcon(AlertType type) const {
    switch (type) {
    case ALERT_DETECTED:
        return "!";
    case ALERT_DANGER:
        return "!";
    case ALERT_WARNING:
        return "!";
    case ALERT_CRITICAL:
        return "!";
    case ALERT_STEALTH:
        return "S";
    case ALERT_SPELL:
        return "*";
    case ALERT_EFFECT:
        return "E";
    default:
        return "?";
    }
}

float UIAlertNotification::getAlertScale(AlertPriority priority) const {
    switch (priority) {
    case PRIORITY_CRITICAL:
        return 1.2f;
    case PRIORITY_HIGH:
        return 1.0f;
    case PRIORITY_NORMAL:
        return 0.8f;
    default:
        return 0.6f;
    }
}

float UIAlertNotification::getCurrentAlpha() const {
    if (!currentAlert_) return 0.0f;

    float progress = currentAlert_->elapsedTime / currentAlert_->duration;

    // Fade out in last 0.5 seconds
    if (progress > 0.83f) {
        float fadeProgress = (progress - 0.83f) / 0.17f;
        return 1.0f - fadeProgress;
    }

    return 1.0f;
}

void UIAlertNotification::processQueue() {
    if (!alertQueue_.empty()) {
        currentAlert_ = alertQueue_.front();
        alertQueue_.erase(alertQueue_.begin());
    } else {
        currentAlert_.reset();
    }
}
