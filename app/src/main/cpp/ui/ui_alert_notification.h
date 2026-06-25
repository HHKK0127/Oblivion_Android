#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief 警告状態通知表示
 *
 * Phase 22: ゲーム内HUD - 警告通知システム
 * 敵検知、警報、重要な状態変化を画面に表示
 */
class UIAlertNotification {
public:
    enum AlertType {
        ALERT_NONE = 0,
        ALERT_DETECTED = 1,
        ALERT_DANGER = 2,
        ALERT_WARNING = 3,
        ALERT_CRITICAL = 4,
        ALERT_STEALTH = 5,
        ALERT_SPELL = 6,
        ALERT_EFFECT = 7
    };

    enum AlertPriority {
        PRIORITY_LOW = 0,
        PRIORITY_NORMAL = 1,
        PRIORITY_HIGH = 2,
        PRIORITY_CRITICAL = 3
    };

    struct Alert {
        std::string message;
        AlertType type;
        AlertPriority priority;
        float duration;
        float elapsedTime;
        glm::vec3 color;
        bool isActive;

        Alert() : message(""), type(ALERT_NONE), priority(PRIORITY_LOW),
                  duration(3.0f), elapsedTime(0.0f), color(1.0f),
                  isActive(true) {}
    };

    UIAlertNotification();
    ~UIAlertNotification() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Add alert
    void addAlert(const std::string& message, AlertType type,
                  AlertPriority priority = PRIORITY_NORMAL, float duration = 3.0f);

    // Get current alert
    const Alert* getCurrentAlert() const { return currentAlert_.get(); }
    bool hasActiveAlert() const { return currentAlert_ != nullptr && currentAlert_->isActive; }

    // Clear alerts
    void dismissAlert();
    void clearAllAlerts();

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    std::shared_ptr<Alert> currentAlert_;
    std::vector<std::shared_ptr<Alert>> alertQueue_;

    static constexpr float ALERT_WIDTH = 300.0f;
    static constexpr float ALERT_HEIGHT = 60.0f;
    static constexpr float ALERT_X = 540.0f;  // Center screen
    static constexpr float ALERT_Y = 500.0f;  // Below level progress

    static constexpr int MAX_QUEUE_SIZE = 5;

    void renderAlert();
    void renderAlertIcon();
    void renderAlertText();
    void processQueue();

    glm::vec3 getAlertColor(AlertType type, AlertPriority priority) const;
    std::string getAlertIcon(AlertType type) const;
    float getAlertScale(AlertPriority priority) const;
    float getCurrentAlpha() const;
};
