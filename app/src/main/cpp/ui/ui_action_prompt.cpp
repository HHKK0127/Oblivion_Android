#include "ui_action_prompt.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

UIActionPrompt::UIActionPrompt() = default;

bool UIActionPrompt::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer_ = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIActionPrompt::addAction(ActionType type, const std::string& label,
                                const std::string& description, char keyCode, float distance) {
    if (actions_.size() >= MAX_ACTIONS) {
        return;  // Max actions reached
    }

    ActionButton button;
    button.type = type;
    button.label = label;
    button.description = description;
    button.keyCode = keyCode;
    button.distance = distance;

    actions_.push_back(button);
    sortActionsByDistance();
}

void UIActionPrompt::clearActions() {
    actions_.clear();
}

const UIActionPrompt::ActionButton* UIActionPrompt::getAction(int index) const {
    if (index >= 0 && index < static_cast<int>(actions_.size())) {
        return &actions_[index];
    }
    return nullptr;
}

const UIActionPrompt::ActionButton* UIActionPrompt::getPrimaryAction() const {
    if (!actions_.empty()) {
        return &actions_[0];  // First action after sorting by distance
    }
    return nullptr;
}

void UIActionPrompt::update(float deltaTime) {
    // Prompts update handled in render
}

void UIActionPrompt::render() {
    if (actions_.empty() || !textRenderer_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderActionButtons();
    renderActionDescription();
    renderKeyGuide();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIActionPrompt::renderActionButtons() {
    // Calculate layout
    float totalWidth = actions_.size() * BUTTON_WIDTH + (actions_.size() - 1) * BUTTON_GAP;
    float startX = START_X - totalWidth / 2.0f;
    float startY = START_Y;

    for (size_t i = 0; i < actions_.size(); i++) {
        const ActionButton& action = actions_[i];
        float buttonX = startX + i * (BUTTON_WIDTH + BUTTON_GAP);
        float buttonY = startY;

        glm::vec3 actionColor = getActionColor(action.type);

        // Button background
        glm::vec4 bgColor(actionColor.x * 0.3f, actionColor.y * 0.3f, actionColor.z * 0.3f, 0.7f);
        UIDrawHelper::drawColoredQuad(buttonX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
            bgColor, screenWidth, screenHeight);

        // Button border
        glm::vec4 borderColor(actionColor, 0.9f);
        UIDrawHelper::drawBorder(buttonX, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT, 1.5f,
            borderColor, screenWidth, screenHeight);

        // Key code
        std::string keyStr(1, action.keyCode);
        textRenderer_->renderText(keyStr,
            buttonX + BUTTON_WIDTH / 2.0f - 3.0f, buttonY + 6.0f,
            actionColor, 0.8f);

        // Action symbol
        std::string symbol = getActionSymbol(action.type);
        if (!symbol.empty()) {
            textRenderer_->renderText(symbol,
                buttonX + 5.0f, buttonY + 10.0f,
                actionColor, 0.6f);
        }
    }
}

void UIActionPrompt::renderActionDescription() {
    if (actions_.empty() || !textRenderer_) return;

    const ActionButton& primaryAction = actions_[0];
    float descY = START_Y + BUTTON_HEIGHT + 20.0f;

    // Primary action label
    glm::vec3 actionColor = getActionColor(primaryAction.type);
    textRenderer_->renderText(primaryAction.label,
        START_X - 50.0f, descY,
        actionColor, 0.8f);

    // Description
    if (!primaryAction.description.empty()) {
        textRenderer_->renderText(primaryAction.description,
            START_X - 50.0f, descY + 15.0f,
            PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
    }

    // Distance indicator
    if (primaryAction.distance > 0.0f) {
        std::string distStr = "Distance: " + std::to_string(static_cast<int>(primaryAction.distance)) + "m";
        textRenderer_->renderText(distStr,
            START_X - 50.0f, descY + 28.0f,
            glm::vec3(0.7f, 0.7f, 0.7f), 0.5f);
    }
}

void UIActionPrompt::renderKeyGuide() {
    if (actions_.empty() || !textRenderer_) return;

    float guideY = START_Y - 30.0f;

    textRenderer_->renderText("Available Actions:",
        START_X - 60.0f, guideY,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 0.7f);
}

glm::vec3 UIActionPrompt::getActionColor(ActionType type) const {
    switch (type) {
    case ACTION_TALK:
        return glm::vec3(0.3f, 0.8f, 0.8f);  // Cyan
    case ACTION_OPEN:
        return glm::vec3(0.9f, 0.6f, 0.2f);  // Orange
    case ACTION_TAKE:
        return glm::vec3(0.9f, 0.8f, 0.3f);  // Gold
    case ACTION_LOOT:
        return glm::vec3(0.8f, 0.6f, 0.2f);  // Brown
    case ACTION_ATTACK:
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    case ACTION_INTERACT:
        return glm::vec3(0.5f, 0.8f, 0.5f);  // Light green
    case ACTION_ACTIVATE:
        return glm::vec3(0.6f, 0.8f, 1.0f);  // Light blue
    case ACTION_EXAMINE:
        return glm::vec3(0.8f, 0.8f, 0.8f);  // Light gray
    case ACTION_PICKPOCKET:
        return glm::vec3(0.7f, 0.3f, 0.3f);  // Dark red
    default:
        return glm::vec3(1.0f, 1.0f, 1.0f);  // White
    }
}

std::string UIActionPrompt::getActionSymbol(ActionType type) const {
    switch (type) {
    case ACTION_TALK:
        return "H";  // Chat
    case ACTION_OPEN:
        return "D";  // Door
    case ACTION_TAKE:
        return "T";  // Take
    case ACTION_LOOT:
        return "L";  // Loot
    case ACTION_ATTACK:
        return "A";  // Attack
    case ACTION_INTERACT:
        return "I";  // Interact
    case ACTION_ACTIVATE:
        return "P";  // Power
    case ACTION_EXAMINE:
        return "E";  // Examine
    case ACTION_PICKPOCKET:
        return "S";  // Steal
    default:
        return "";
    }
}

void UIActionPrompt::sortActionsByDistance() {
    std::sort(actions_.begin(), actions_.end(),
        [](const ActionButton& a, const ActionButton& b) {
            return a.distance < b.distance;
        });
}
