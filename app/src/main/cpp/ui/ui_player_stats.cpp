#include "ui_player_stats.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "placeholder_assets.h"
#include <glm/glm.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>

UIPlayerStats::UIPlayerStats() = default;

bool UIPlayerStats::initialize(TextRenderer* tr, int screenW, int screenH) {
    if (!tr) return false;

    textRenderer = tr;
    screenWidth = screenW;
    screenHeight = screenH;

    return true;
}

void UIPlayerStats::setEquipment(int slot, const std::string& itemName, int durability, int maxDurability) {
    if (slot >= 0 && slot < 4) {
        stats_.armor[slot].itemName = itemName;
        stats_.armor[slot].durability = durability;
        stats_.armor[slot].maxDurability = maxDurability;
        stats_.armor[slot].equipped = true;
    }
}

void UIPlayerStats::setMainWeapon(const std::string& weaponName, int durability, int maxDurability) {
    stats_.weapon.itemName = weaponName;
    stats_.weapon.durability = durability;
    stats_.weapon.maxDurability = maxDurability;
    stats_.weapon.equipped = true;
}

void UIPlayerStats::update(float deltaTime) {
    if (expanded_ && expandAnimTime_ < EXPAND_DURATION) {
        expandAnimTime_ += deltaTime;
    } else if (!expanded_ && expandAnimTime_ > 0.0f) {
        expandAnimTime_ -= deltaTime;
    }

    if (expandAnimTime_ < 0.0f) expandAnimTime_ = 0.0f;
    if (expandAnimTime_ > EXPAND_DURATION) expandAnimTime_ = EXPAND_DURATION;
}

void UIPlayerStats::render() {
    if (!textRenderer) return;

    renderCompactPanel();
    if (expanded_ || expandAnimTime_ > 0.0f) {
        renderExpandedPanel();
    }
}

void UIPlayerStats::renderCompactPanel() {
    float x = START_X;
    float y = START_Y;
    float panelHeight = PANEL_HEIGHT;

    // Background panel with border
    glm::vec4 bgColor(0.15f, 0.12f, 0.08f, 0.7f);
    UIDrawHelper::drawColoredQuad(x - 5.0f, y - 5.0f, PANEL_WIDTH + 10.0f, panelHeight + 10.0f,
                                  bgColor, screenWidth, screenHeight);

    glm::vec4 borderColor(0.5f, 0.5f, 0.5f, 0.4f);
    UIDrawHelper::drawBorder(x - 5.0f, y - 5.0f, PANEL_WIDTH + 10.0f, panelHeight + 10.0f, 1.0f,
                             borderColor, screenWidth, screenHeight);

    renderGoldInfo();
    renderWeightBar();
    renderCarryingStatus();
}

void UIPlayerStats::renderExpandedPanel() {
    float animProgress = expandAnimTime_ / EXPAND_DURATION;
    if (!expanded_) {
        animProgress = 1.0f - animProgress;
    }

    float x = START_X;
    float y = START_Y + PANEL_HEIGHT + 10.0f;
    float expandedHeight = (PANEL_EXPANDED_HEIGHT - PANEL_HEIGHT) * animProgress;

    if (expandedHeight > 5.0f) {
        // Semi-transparent background for expanded content
        glm::vec4 bgColor(0.15f, 0.12f, 0.08f, 0.7f);
        UIDrawHelper::drawColoredQuad(x - 5.0f, y, PANEL_WIDTH + 10.0f, expandedHeight,
                                      bgColor, screenWidth, screenHeight);

        glm::vec4 borderColor(0.5f, 0.5f, 0.5f, 0.4f);
        UIDrawHelper::drawBorder(x - 5.0f, y, PANEL_WIDTH + 10.0f, expandedHeight, 1.0f,
                                 borderColor, screenWidth, screenHeight);

        // Render Equipment inside the expanded panel if fully expanded
        if (animProgress > 0.9f) {
            renderEquipmentInfo();
        }
    }
}

void UIPlayerStats::renderGoldInfo() {
    float x = START_X + 10.0f;
    float y = START_Y + 8.0f;

    textRenderer->renderText("Gold:", x, y, glm::vec3(1.0f, 0.84f, 0.0f), 0.5f);

    std::ostringstream goldStr;
    goldStr << stats_.gold;
    textRenderer->renderText(goldStr.str(), x + 50.0f, y, glm::vec3(1.0f, 0.84f, 0.0f), 0.5f);
}

void UIPlayerStats::renderWeightBar() {
    float x = START_X + 10.0f;
    float y = START_Y + 30.0f;

    textRenderer->renderText("Weight:", x, y, glm::vec3(0.8f, 0.8f, 0.8f), 0.45f);

    // Weight bar background
    glm::vec4 bgBarColor(0.2f, 0.2f, 0.2f, 0.8f);
    UIDrawHelper::drawColoredQuad(x, y + 12.0f, WEIGHT_BAR_WIDTH, WEIGHT_BAR_HEIGHT,
                                  bgBarColor, screenWidth, screenHeight);

    // Fill bar
    float weightPercent = stats_.maxCarryWeight > 0.0f ? stats_.inventoryWeight / stats_.maxCarryWeight : 0.0f;
    weightPercent = std::max(0.0f, std::min(weightPercent, 1.0f));
    float fillWidth = weightPercent * WEIGHT_BAR_WIDTH;
    glm::vec3 barColorVec = getWeightColor();
    glm::vec4 fillBarColor(barColorVec.x, barColorVec.y, barColorVec.z, 0.8f);
    UIDrawHelper::drawColoredQuad(x, y + 12.0f, fillWidth, WEIGHT_BAR_HEIGHT,
                                  fillBarColor, screenWidth, screenHeight);

    // Border
    glm::vec4 borderBarColor(barColorVec.x, barColorVec.y, barColorVec.z, 1.0f);
    UIDrawHelper::drawBorder(x, y + 12.0f, WEIGHT_BAR_WIDTH, WEIGHT_BAR_HEIGHT, 1.0f,
                             borderBarColor, screenWidth, screenHeight);

    // Weight text
    std::ostringstream weightStr;
    weightStr << std::fixed << std::setprecision(1) << stats_.inventoryWeight << "/" << stats_.maxCarryWeight;
    textRenderer->renderText(weightStr.str(), x, y + 26.0f, glm::vec3(0.8f, 0.8f, 0.8f), 0.4f);
}

void UIPlayerStats::renderEquipmentInfo() {
    float x = START_X + 10.0f;
    float y = START_Y + PANEL_HEIGHT + 15.0f;

    const char* slotNames[] = {"Head", "Chest", "Hands", "Feet"};

    textRenderer->renderText("EQUIPMENT", x, y, glm::vec3(1.0f, 1.0f, 0.0f), 0.5f);
    y += 15.0f;

    for (int i = 0; i < 4; i++) {
        std::string name = stats_.armor[i].equipped ? stats_.armor[i].itemName : "Empty";
        std::string label = std::string(slotNames[i]) + ": " + name;
        textRenderer->renderText(label, x, y, glm::vec3(0.9f, 0.9f, 0.9f), 0.4f);
        y += 12.0f;
    }

    std::string weaponName = stats_.weapon.equipped ? stats_.weapon.itemName : "Empty";
    std::string weaponLabel = "Weapon: " + weaponName;
    textRenderer->renderText(weaponLabel, x, y, glm::vec3(0.9f, 0.9f, 0.9f), 0.4f);
}

void UIPlayerStats::renderCarryingStatus() {
    float x = START_X + 10.0f;
    float y = START_Y + 75.0f;

    std::string statusText = getCarryingStatusText();
    glm::vec3 statusColor = getWeightColor();

    textRenderer->renderText(statusText, x, y, statusColor, 0.45f);
}

glm::vec3 UIPlayerStats::getWeightColor() const {
    float weightPercent = stats_.maxCarryWeight > 0.0f ? stats_.inventoryWeight / stats_.maxCarryWeight : 0.0f;

    if (weightPercent > 1.0f) {
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red - Overencumbered
    } else if (weightPercent > 0.75f) {
        return glm::vec3(1.0f, 0.65f, 0.0f);  // Orange - Heavy
    } else {
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green - Normal
    }
}

std::string UIPlayerStats::getCarryingStatusText() const {
    float weightPercent = stats_.maxCarryWeight > 0.0f ? stats_.inventoryWeight / stats_.maxCarryWeight : 0.0f;

    if (weightPercent > 1.0f) {
        return "Overencumbered";
    } else if (weightPercent > 0.75f) {
        return "Heavy";
    } else {
        return "Normal";
    }
}

float UIPlayerStats::getExpandedPanelHeight() const {
    return PANEL_HEIGHT + (PANEL_EXPANDED_HEIGHT - PANEL_HEIGHT) * (expandAnimTime_ / EXPAND_DURATION);
}
