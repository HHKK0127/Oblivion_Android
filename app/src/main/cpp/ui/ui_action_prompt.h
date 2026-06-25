#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief アクションプロンプト表示
 *
 * Phase 19: ゲーム内HUD - インタラクション表示
 * 話しかける、開ける、盗む、攻撃するなどのアクション可能を表示
 */
class UIActionPrompt {
public:
    enum ActionType {
        ACTION_NONE = 0,
        ACTION_TALK = 1,
        ACTION_OPEN = 2,
        ACTION_TAKE = 3,
        ACTION_LOOT = 4,
        ACTION_ATTACK = 5,
        ACTION_INTERACT = 6,
        ACTION_ACTIVATE = 7,
        ACTION_EXAMINE = 8,
        ACTION_PICKPOCKET = 9
    };

    struct ActionButton {
        ActionType type;
        std::string label;
        std::string description;
        char keyCode;  // 'E', 'Q', etc.
        float distance;  // Distance to target

        ActionButton() : type(ACTION_NONE), label(""), description(""),
                         keyCode('E'), distance(0.0f) {}
    };

    UIActionPrompt();
    ~UIActionPrompt() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Add action button
    void addAction(ActionType type, const std::string& label,
                   const std::string& description, char keyCode, float distance);

    // Clear all actions
    void clearActions();

    // Get action at index
    const ActionButton* getAction(int index) const;
    int getActionCount() const { return actions_.size(); }

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    // Get primary action (closest or most important)
    const ActionButton* getPrimaryAction() const;

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    std::vector<ActionButton> actions_;

    static constexpr float BUTTON_WIDTH = 80.0f;
    static constexpr float BUTTON_HEIGHT = 30.0f;
    static constexpr float BUTTON_GAP = 10.0f;
    static constexpr float START_Y = 150.0f;  // Below compass
    static constexpr float START_X = 540.0f;  // Center screen

    static constexpr int MAX_ACTIONS = 6;

    void renderActionButtons();
    void renderActionDescription();
    void renderKeyGuide();

    glm::vec3 getActionColor(ActionType type) const;
    std::string getActionSymbol(ActionType type) const;

    void sortActionsByDistance();
};
