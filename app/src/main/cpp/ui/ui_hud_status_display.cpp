#include "ui_hud_status_display.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include "../game/character_status.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

UIHudStatusDisplay::UIHudStatusDisplay() = default;

bool UIHudStatusDisplay::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer_ = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIHudStatusDisplay::update(float deltaTime) {
    // Status updates handled by game systems
}

void UIHudStatusDisplay::render() {
    if (!textRenderer_ || !character_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float x = START_X;
    float y = START_Y;

    // Background panel for status display
    float panelWidth = BAR_WIDTH + 40.0f;
    float panelHeight = BAR_HEIGHT * 3 + BAR_GAP * 2 + 50.0f;
    glm::vec4 panelColor(0.15f, 0.12f, 0.08f, 0.7f);
    UIDrawHelper::drawColoredQuad(x - 5.0f, y - 5.0f, panelWidth, panelHeight,
        panelColor, screenWidth, screenHeight);

    // Panel border
    glm::vec4 borderColor(0.5f, 0.4f, 0.2f, 0.9f);
    UIDrawHelper::drawBorder(x - 5.0f, y - 5.0f, panelWidth, panelHeight, 1.0f,
        borderColor, screenWidth, screenHeight);

    // Render bars
    renderHealthBar(x, y);
    renderManaBar(x, y + BAR_HEIGHT + BAR_GAP);
    renderStaminaBar(x, y + (BAR_HEIGHT + BAR_GAP) * 2);

    // Render level info
    renderLevelInfo(x, y + (BAR_HEIGHT + BAR_GAP) * 3 + 10.0f);

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIHudStatusDisplay::renderBar(float x, float y, float currentValue, float maxValue,
                                    glm::vec3 barColor, const std::string& label) {
    if (!textRenderer_ || maxValue <= 0.0f) return;

    // Label text
    textRenderer_->renderText(label,
        x, y - 8.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);

    // Background bar
    glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
    UIDrawHelper::drawColoredQuad(x, y, BAR_WIDTH, BAR_HEIGHT, bgColor,
        screenWidth, screenHeight);

    // Value bar
    float fillWidth = (currentValue / maxValue) * BAR_WIDTH;
    fillWidth = std::max(0.0f, std::min(fillWidth, BAR_WIDTH));
    glm::vec4 fillColor(barColor, 0.9f);
    UIDrawHelper::drawColoredQuad(x, y, fillWidth, BAR_HEIGHT, fillColor,
        screenWidth, screenHeight);

    // Border
    glm::vec4 borderColor(barColor, 1.0f);
    UIDrawHelper::drawBorder(x, y, BAR_WIDTH, BAR_HEIGHT, 1.0f, borderColor,
        screenWidth, screenHeight);

    // Value text
    std::stringstream ss;
    ss << static_cast<int>(currentValue) << "/" << static_cast<int>(maxValue);
    std::string valueText = ss.str();
    textRenderer_->renderText(valueText,
        x + BAR_WIDTH + 5.0f, y + 2.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
}

void UIHudStatusDisplay::renderHealthBar(float x, float y) {
    if (!character_) return;
    float healthPercent = character_->health / std::max(1.0f, character_->maxHealth);
    glm::vec3 healthColor = getHealthColor(healthPercent);
    renderBar(x, y, character_->health, character_->maxHealth, healthColor, "Health");
}

void UIHudStatusDisplay::renderManaBar(float x, float y) {
    if (!character_) return;
    float manaPercent = character_->mana / std::max(1.0f, character_->maxMana);
    glm::vec3 manaColor = getManaColor(manaPercent);
    renderBar(x, y, character_->mana, character_->maxMana, manaColor, "Mana");
}

void UIHudStatusDisplay::renderStaminaBar(float x, float y) {
    if (!character_) return;
    float staminaPercent = character_->stamina / std::max(1.0f, character_->maxStamina);
    glm::vec3 staminaColor = getStaminaColor(staminaPercent);
    renderBar(x, y, character_->stamina, character_->maxStamina, staminaColor, "Stamina");
}

void UIHudStatusDisplay::renderLevelInfo(float x, float y) {
    if (!character_ || !textRenderer_) return;

    std::stringstream ss;
    ss << "Level " << character_->level;
    std::string levelText = ss.str();

    textRenderer_->renderText(levelText,
        x, y,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 0.7f);

    std::stringstream expSs;
    expSs << "EXP: " << character_->experience;
    std::string expText = expSs.str();

    textRenderer_->renderText(expText,
        x, y + 12.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
}

glm::vec3 UIHudStatusDisplay::getHealthColor(float healthPercent) const {
    if (healthPercent > 0.5f) {
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green
    } else if (healthPercent > 0.25f) {
        return glm::vec3(1.0f, 0.7f, 0.0f);  // Orange
    } else {
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    }
}

glm::vec3 UIHudStatusDisplay::getManaColor(float manaPercent) const {
    if (manaPercent > 0.5f) {
        return glm::vec3(0.3f, 0.5f, 1.0f);  // Bright blue
    } else if (manaPercent > 0.25f) {
        return glm::vec3(0.2f, 0.3f, 0.8f);  // Medium blue
    } else {
        return glm::vec3(0.1f, 0.1f, 0.5f);  // Dark blue
    }
}

glm::vec3 UIHudStatusDisplay::getStaminaColor(float staminaPercent) const {
    if (staminaPercent > 0.5f) {
        return glm::vec3(0.8f, 0.6f, 0.2f);  // Gold
    } else if (staminaPercent > 0.25f) {
        return glm::vec3(0.9f, 0.7f, 0.0f);  // Orange-gold
    } else {
        return glm::vec3(0.7f, 0.5f, 0.0f);  // Dark gold
    }
}
