#include "ui_target_info.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

UITargetInfo::UITargetInfo() = default;

bool UITargetInfo::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UITargetInfo::setTarget(const TargetData& target) {
    currentTarget_ = target;
    targetLockDuration_ = 0.0f;
}

void UITargetInfo::clearTarget() {
    currentTarget_ = TargetData();
    targetLockDuration_ = 0.0f;
}

void UITargetInfo::updateTargetHealth(float currentHealth) {
    if (hasTarget()) {
        currentTarget_.health = std::max(0.0f, std::min(currentHealth, currentTarget_.maxHealth));
    }
}

void UITargetInfo::update(float deltaTime) {
    if (hasTarget()) {
        targetLockDuration_ += deltaTime;
    }
}

void UITargetInfo::render() {
    if (!hasTarget() || !textRenderer) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderTargetPanel();
    renderTargetName();
    renderHealthBar();
    renderTargetInfo();
    renderHostileIndicator();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UITargetInfo::renderTargetPanel() {
    // Target info background panel
    float panelX = PANEL_X;
    float panelY = PANEL_Y;

    glm::vec4 bgColor(0.15f, 0.12f, 0.08f, 0.85f);
    UIDrawHelper::drawColoredQuad(panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT,
        bgColor, screenWidth, screenHeight);

    // Panel border
    glm::vec4 borderColor(0.5f, 0.5f, 0.5f, 0.45f);
    if (currentTarget_.isHostile) {
        borderColor = glm::vec4(1.0f, 0.2f, 0.2f, 0.95f);
    }
    UIDrawHelper::drawBorder(panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT, 2.0f,
        borderColor, screenWidth, screenHeight);
}

void UITargetInfo::renderTargetName() {
    if (!textRenderer) return;

    float panelX = PANEL_X;
    float panelY = PANEL_Y;

    glm::vec3 nameColor = getNameColor(currentTarget_.type, currentTarget_.isHostile);
    textRenderer->renderText(currentTarget_.name,
        panelX + HEALTH_BAR_PADDING, panelY + 8.0f,
        nameColor, 0.8f);

    // Faction name if available
    if (!currentTarget_.faction.empty()) {
        textRenderer->renderText(currentTarget_.faction,
            panelX + HEALTH_BAR_PADDING, panelY + 20.0f,
            PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
    }
}

void UITargetInfo::renderHealthBar() {
    if (!textRenderer) return;

    float panelX = PANEL_X;
    float panelY = PANEL_Y;

    float healthBarX = panelX + HEALTH_BAR_PADDING;
    float healthBarY = panelY + 35.0f;

    float healthPercent = currentTarget_.maxHealth > 0.0f
        ? currentTarget_.health / currentTarget_.maxHealth
        : 0.0f;

    glm::vec3 barColor = getHealthBarColor(healthPercent);
    renderBar(healthBarX, healthBarY, currentTarget_.health,
        currentTarget_.maxHealth, barColor, "");

    // Health percentage text
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << (healthPercent * 100.0f) << "%";
    std::string healthText = ss.str();

    textRenderer->renderText(healthText,
        healthBarX + HEALTH_BAR_WIDTH - 30.0f, healthBarY + 3.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
}

void UITargetInfo::renderTargetInfo() {
    if (!textRenderer) return;

    float panelX = PANEL_X;
    float panelY = PANEL_Y;

    std::stringstream ss;
    ss << "Lvl " << currentTarget_.level;
    std::string levelText = ss.str();

    textRenderer->renderText(levelText,
        panelX + HEALTH_BAR_PADDING, panelY + 65.0f,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 0.6f);

    // Target type indicator
    std::string typeText = "";
    glm::vec3 typeColor = glm::vec3(0.8f, 0.8f, 0.8f);

    switch (currentTarget_.type) {
    case TARGET_NPC:
        typeText = "[NPC]";
        typeColor = glm::vec3(0.3f, 0.8f, 0.3f);
        break;
    case TARGET_ENEMY:
        typeText = "[ENEMY]";
        typeColor = glm::vec3(1.0f, 0.2f, 0.2f);
        break;
    case TARGET_CREATURE:
        typeText = "[CREATURE]";
        typeColor = glm::vec3(0.9f, 0.6f, 0.2f);
        break;
    default:
        break;
    }

    if (!typeText.empty()) {
        textRenderer->renderText(typeText,
            panelX + HEALTH_BAR_PADDING + 60.0f, panelY + 65.0f,
            typeColor, 0.5f);
    }
}

void UITargetInfo::renderHostileIndicator() {
    if (!currentTarget_.isHostile || !textRenderer) return;

    float panelX = PANEL_X;
    float panelY = PANEL_Y;

    // Red "!" indicator for hostile targets
    textRenderer->renderText("!",
        panelX + PANEL_WIDTH - 20.0f, panelY + 8.0f,
        glm::vec3(1.0f, 0.2f, 0.2f), 1.0f);
}

void UITargetInfo::renderBar(float x, float y, float currentValue, float maxValue,
                              glm::vec3 barColor, const std::string& label) {
    // Background bar
    glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
    UIDrawHelper::drawColoredQuad(x, y, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT,
        bgColor, screenWidth, screenHeight);

    // Value bar
    float fillWidth = (currentValue / std::max(1.0f, maxValue)) * HEALTH_BAR_WIDTH;
    fillWidth = std::max(0.0f, std::min(fillWidth, HEALTH_BAR_WIDTH));
    glm::vec4 fillColor(barColor.x, barColor.y, barColor.z, 0.9f);
    UIDrawHelper::drawColoredQuad(x, y, fillWidth, HEALTH_BAR_HEIGHT,
        fillColor, screenWidth, screenHeight);

    // Border
    glm::vec4 borderColor(barColor.x, barColor.y, barColor.z, 1.0f);
    UIDrawHelper::drawBorder(x, y, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT, 1.0f,
        borderColor, screenWidth, screenHeight);
}

glm::vec3 UITargetInfo::getHealthBarColor(float healthPercent) const {
    if (healthPercent > 0.5f) {
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green
    } else if (healthPercent > 0.25f) {
        return glm::vec3(1.0f, 0.7f, 0.0f);  // Orange
    } else {
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    }
}

glm::vec3 UITargetInfo::getNameColor(TargetType type, bool isHostile) const {
    if (isHostile) {
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red for hostile
    }

    switch (type) {
    case TARGET_NPC:
        return PlaceholderAssets::Colors::PARCHMENT_LIGHT;
    case TARGET_ENEMY:
        return glm::vec3(1.0f, 0.4f, 0.2f);  // Orange-red
    case TARGET_CREATURE:
        return glm::vec3(0.9f, 0.6f, 0.2f);  // Gold
    default:
        return glm::vec3(1.0f, 1.0f, 1.0f);  // White
    }
}
