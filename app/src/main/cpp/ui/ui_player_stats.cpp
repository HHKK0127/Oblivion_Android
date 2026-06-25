#include "ui_player_stats.h"
#include "text_renderer.h"
#include <glm/glm.hpp>
#include <sstream>
#include <iomanip>

UIPlayerStats::UIPlayerStats() = default;

bool UIPlayerStats::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;

    textRenderer_ = textRenderer;
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
    if (!textRenderer_) return;

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
    textRenderer_->drawRectFilled(x, y, PANEL_WIDTH, panelHeight, glm::vec4(0.1f, 0.08f, 0.05f, 0.85f));
    textRenderer_->drawRect(x, y, PANEL_WIDTH, panelHeight, glm::vec4(0.7f, 0.65f, 0.5f, 0.9f), 2.0f);

    float textX = x + 10.0f;
    float textY = y + 8.0f;

    // Gold amount
    textRenderer_->drawText(textX, textY, "Gold:", glm::vec3(1.0f, 0.84f, 0.0f), 0.5f);
    std::ostringstream goldStr;
    goldStr << stats_.gold;
    textRenderer_->drawText(textX + 50.0f, textY, goldStr.str(), glm::vec3(1.0f, 0.84f, 0.0f), 0.5f);

    // Weight bar and status
    renderWeightBar();
    renderCarryingStatus();
}

void UIPlayerStats::renderExpandedPanel() {
    float animProgress = expandAnimTime_ / EXPAND_DURATION;
    if (!expanded_) {
        animProgress = 1.0f - animProgress;
    }

    float x = START_X;
    float y = START_Y - (PANEL_EXPANDED_HEIGHT - PANEL_HEIGHT) * animProgress;
    float panelHeight = PANEL_HEIGHT + (PANEL_EXPANDED_HEIGHT - PANEL_HEIGHT) * animProgress;

    // Semi-transparent background for expanded content
    if (panelHeight > PANEL_HEIGHT + 5.0f) {
        textRenderer_->drawRectFilled(x, y + PANEL_HEIGHT, PANEL_WIDTH, panelHeight - PANEL_HEIGHT,
                                     glm::vec4(0.1f, 0.08f, 0.05f, 0.7f));
    }

    float textX = x + 10.0f;
    float textY = y + PANEL_HEIGHT + 8.0f;

    // Equipment info
    renderEquipmentInfo();

    // Spell slots info
    if (panelHeight > PANEL_HEIGHT + 50.0f) {
        std::ostringstream spellStr;
        spellStr << "Spells: " << stats_.activeSpells << "/" << stats_.maxActiveSpells;
        textRenderer_->drawText(textX, textY + 55.0f, spellStr.str(), glm::vec3(0.6f, 0.8f, 1.0f), 0.45f);
    }
}

void UIPlayerStats::renderGoldInfo() {
    float x = START_X + 10.0f;
    float y = START_Y + 8.0f;

    textRenderer_->drawText(x, y, "Gold:", glm::vec3(1.0f, 0.84f, 0.0f), 0.5f);

    std::ostringstream goldStr;
    goldStr << stats_.gold;
    textRenderer_->drawText(x + 50.0f, y, goldStr.str(), glm::vec3(1.0f, 0.84f, 0.0f), 0.5f);
}

void UIPlayerStats::renderWeightBar() {
    float x = START_X + 10.0f;
    float y = START_Y + 30.0f;

    textRenderer_->drawText(x, y, "Weight:", glm::vec3(0.8f, 0.8f, 0.8f), 0.45f);

    // Weight bar background
    textRenderer_->drawRectFilled(x, y + 12.0f, WEIGHT_BAR_WIDTH, WEIGHT_BAR_HEIGHT,
                                 glm::vec4(0.2f, 0.2f, 0.2f, 0.6f));

    // Weight bar fill
    float weightPercent = stats_.maxCarryWeight > 0 ? stats_.inventoryWeight / stats_.maxCarryWeight : 0.0f;
    if (weightPercent > 1.0f) weightPercent = 1.0f;

    glm::vec3 barColor = getWeightColor();
    textRenderer_->drawRectFilled(x, y + 12.0f, WEIGHT_BAR_WIDTH * weightPercent, WEIGHT_BAR_HEIGHT,
                                 glm::vec4(barColor.x, barColor.y, barColor.z, 0.8f));

    // Weight text
    std::ostringstream weightStr;
    weightStr << std::fixed << std::setprecision(1) << stats_.inventoryWeight << "/" << stats_.maxCarryWeight;
    textRenderer_->drawText(x + WEIGHT_BAR_WIDTH + 5.0f, y + 12.0f, weightStr.str(), glm::vec3(0.8f, 0.8f, 0.8f), 0.4f);
}

void UIPlayerStats::renderEquipmentInfo() {
    float x = START_X + 10.0f;
    float y = START_Y + PANEL_HEIGHT + 8.0f;

    const char* slotNames[] = {"Head", "Chest", "Hands", "Feet"};

    for (int i = 0; i < 4; i++) {
        if (i < 2) {
            textRenderer_->drawText(x, y + (i * 15.0f), slotNames[i], glm::vec3(0.8f, 0.8f, 0.8f), 0.4f);
            textRenderer_->drawText(x + 50.0f, y + (i * 15.0f), stats_.armor[i].itemName, glm::vec3(0.9f, 0.9f, 0.9f), 0.4f);
        } else {
            textRenderer_->drawText(x + WEIGHT_BAR_WIDTH / 2 + 5.0f, y + ((i - 2) * 15.0f), slotNames[i], glm::vec3(0.8f, 0.8f, 0.8f), 0.4f);
            textRenderer_->drawText(x + WEIGHT_BAR_WIDTH / 2 + 55.0f, y + ((i - 2) * 15.0f), stats_.armor[i].itemName, glm::vec3(0.9f, 0.9f, 0.9f), 0.4f);
        }
    }

    // Weapon info
    textRenderer_->drawText(x, y + 60.0f, "Weapon:", glm::vec3(0.8f, 0.8f, 0.8f), 0.4f);
    textRenderer_->drawText(x + 60.0f, y + 60.0f, stats_.weapon.itemName, glm::vec3(0.9f, 0.9f, 0.9f), 0.4f);
}

void UIPlayerStats::renderCarryingStatus() {
    float x = START_X + 10.0f;
    float y = START_Y + 50.0f;

    std::string statusText = getCarryingStatusText();
    glm::vec3 statusColor = getWeightColor();

    textRenderer_->drawText(x, y, statusText, statusColor, 0.45f);
}

glm::vec3 UIPlayerStats::getWeightColor() const {
    float weightPercent = stats_.maxCarryWeight > 0 ? stats_.inventoryWeight / stats_.maxCarryWeight : 0.0f;

    if (weightPercent > 1.0f) {
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red - Overencumbered
    } else if (weightPercent > 0.75f) {
        return glm::vec3(1.0f, 0.65f, 0.0f);  // Orange - Heavy
    } else {
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green - Normal
    }
}

std::string UIPlayerStats::getCarryingStatusText() const {
    float weightPercent = stats_.maxCarryWeight > 0 ? stats_.inventoryWeight / stats_.maxCarryWeight : 0.0f;

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
