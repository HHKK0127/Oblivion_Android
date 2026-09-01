#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <queue>
#include <memory>

/**
 * @brief トースト通知UI
 *
 * Phase 14: 画面上部に一時的に表示される短いメッセージ
 * アイテム入手、クエスト更新、ステータス変更などの
 * 軽い通知に使用
 */
class UIToast {
public:
    enum ToastType {
        ITEM_ACQUIRED = 0,     // Item acquired (gold)
        QUEST_UPDATED = 1,     // Quest updated (blue)
        LEVEL_UP = 2,          // Level up (green)
        NOTIFICATION = 3       // Notification (white)
    };

    struct Toast {
        std::string message;
        ToastType type;
        float duration;  // Display duration in seconds

        Toast(const std::string& msg, ToastType t = NOTIFICATION, float d = 3.0f)
            : message(msg), type(t), duration(d) {}
    };

    UIToast();
    ~UIToast() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Queue a toast notification
    void showToast(const Toast& toast);

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

private:
    TextRenderer* textRenderer = nullptr;
    std::queue<Toast> toastQueue_;
    Toast* currentToast_ = nullptr;
    float elapsedTime_ = 0.0f;
    float fadeAlpha_ = 1.0f;

    int screenWidth = 1080;
    int screenHeight = 1920;

    void displayNextToast();
    void dismissCurrentToast();

    glm::vec3 getToastColor() const;
    glm::vec3 getToastBackgroundColor() const;
    float getToastY() const;
};
