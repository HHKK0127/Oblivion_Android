#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include <memory>
#include <functional>
#include <queue>

/**
 * @brief Message box UI
 *
 * Phase 14: In-game message display and user selection
 * Displays important notifications such as item acquisition, level up, quest completion
 * in parchment-style dialog
 */
class UIMessageBox : public UIPanel {
public:
    enum MessageType {
        INFO = 0,      // Info (blue)
        SUCCESS = 1,   // Success (green)
        WARNING = 2,   // Warning (yellow)
        ERROR = 3      // Error (red)
    };

    enum ButtonType {
        OK = 0,
        YES_NO = 1,
        OK_CANCEL = 2
    };

    using MessageCallback = std::function<void(bool confirmed)>;

    struct Message {
        std::string title;
        std::string text;
        MessageType type;
        ButtonType buttons;
        MessageCallback callback;

        Message(const std::string& t, const std::string& msg,
                MessageType mt = INFO, ButtonType bt = OK,
                MessageCallback cb = nullptr)
            : title(t), text(msg), type(mt), buttons(bt), callback(cb) {}
    };

    explicit UIMessageBox(const std::string& title = "Message");
    ~UIMessageBox() override = default;

    bool initialize(TextRenderer* textRenderer);

    // Queue a message to be displayed
    void queueMessage(const Message& msg);

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

private:
    TextRenderer* textRenderer = nullptr;
    std::queue<Message> messageQueue_;
    Message* currentMessage_ = nullptr;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    void displayNextMessage();
    void dismissCurrentMessage(bool confirmed);

    void renderMessageDialog();
    void renderButtons();
    void renderTypeIndicator();

    int hitTestButton(float x, float y) const;

    // Layout helpers
    float getDialogWidth() const;
    float getDialogHeight() const;
    float getDialogX() const;
    float getDialogY() const;
    float getButtonY() const;

    glm::vec3 getTypeColor() const;
};
