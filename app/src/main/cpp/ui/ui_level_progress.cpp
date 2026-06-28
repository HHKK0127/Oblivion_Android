#include "ui_level_progress.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

UILevelProgress::UILevelProgress() = default;

bool UILevelProgress::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UILevelProgress::updateExperience(int totalExp, int expInCurrentLevel) {
    levelData_.totalExperience = totalExp;
    levelData_.experienceForCurrentLevel = expInCurrentLevel;
}

void UILevelProgress::updateSkillIncreases(int increases) {
    levelData_.skillIncreases = increases;
}

void UILevelProgress::notifyLevelUp(int newLevel) {
    levelData_.currentLevel = newLevel;
    levelUpPending_ = true;
    levelUpAnimationTime_ = 0.0f;
}

void UILevelProgress::update(float deltaTime) {
    if (levelUpPending_) {
        levelUpAnimationTime_ += deltaTime;
        if (levelUpAnimationTime_ >= LEVEL_UP_ANIMATION_DURATION) {
            levelUpPending_ = false;
        }
    }
}

void UILevelProgress::render() {
    if (!textRenderer) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderProgressPanel();
    renderExperienceBar();
    renderLevelInfo();
    renderSkillIncreasesInfo();

    if (levelUpPending_) {
        renderLevelUpNotification();
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UILevelProgress::renderProgressPanel() {
    float panelX = START_X - PANEL_WIDTH / 2.0f;
    float panelY = START_Y;

    // Panel background
    glm::vec4 bgColor(0.15f, 0.12f, 0.08f, 0.8f);
    UIDrawHelper::drawColoredQuad(panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT,
        bgColor, screenWidth, screenHeight);

    // Panel border
    glm::vec4 borderColor(0.5f, 0.5f, 0.5f, 0.4f);
    UIDrawHelper::drawBorder(panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT, 1.5f,
        borderColor, screenWidth, screenHeight);
}

void UILevelProgress::renderExperienceBar() {
    float panelX = START_X - PANEL_WIDTH / 2.0f;
    float barX = panelX + 15.0f;
    float barY = START_Y + 35.0f;

    float expPercent = getExperiencePercent();
    expPercent = std::max(0.0f, std::min(1.0f, expPercent));

    // Background bar
    glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
    UIDrawHelper::drawColoredQuad(barX, barY, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT,
        bgColor, screenWidth, screenHeight);

    // Experience bar
    float fillWidth = PROGRESS_BAR_WIDTH * expPercent;
    glm::vec3 expColor = getProgressColor();
    glm::vec4 fillColor(expColor.x, expColor.y, expColor.z, 0.9f);
    UIDrawHelper::drawColoredQuad(barX, barY, fillWidth, PROGRESS_BAR_HEIGHT,
        fillColor, screenWidth, screenHeight);

    // Border
    glm::vec4 borderColor(expColor.x, expColor.y, expColor.z, 1.0f);
    UIDrawHelper::drawBorder(barX, barY, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, 1.0f,
        borderColor, screenWidth, screenHeight);

    // Progress percentage text
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << (expPercent * 100.0f) << "%";
    std::string percentText = ss.str();

    textRenderer->renderText(percentText,
        barX + PROGRESS_BAR_WIDTH - 35.0f, barY + 1.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
}

void UILevelProgress::renderLevelInfo() {
    float panelX = START_X - PANEL_WIDTH / 2.0f;

    // Current Level
    std::stringstream levelSs;
    levelSs << "Level " << levelData_.currentLevel;
    std::string levelText = levelSs.str();

    textRenderer->renderText(levelText,
        panelX + 15.0f, START_Y + 12.0f,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 0.8f);

    // Experience info
    std::stringstream expSs;
    expSs << levelData_.experienceForCurrentLevel << " / "
          << levelData_.experienceNeededForNextLevel;
    std::string expText = expSs.str();

    textRenderer->renderText(expText,
        panelX + 15.0f, START_Y + 52.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);
}

void UILevelProgress::renderSkillIncreasesInfo() {
    if (levelData_.skillIncreases <= 0) return;

    float panelX = START_X - PANEL_WIDTH / 2.0f;

    std::stringstream ss;
    ss << "Skills: " << levelData_.skillIncreases;
    std::string skillText = ss.str();

    glm::vec3 skillColor = glm::vec3(0.9f, 0.7f, 0.2f);  // Gold
    textRenderer->renderText(skillText,
        panelX + 15.0f, START_Y + 65.0f,
        skillColor, 0.6f);
}

void UILevelProgress::renderLevelUpNotification() {
    // Level up animation effect
    float animProgress = levelUpAnimationTime_ / LEVEL_UP_ANIMATION_DURATION;
    float alpha = 1.0f;

    // Fade out in last 0.5 seconds
    if (animProgress > 0.75f) {
        alpha = 1.0f - ((animProgress - 0.75f) / 0.25f);
    }

    float notifY = START_Y - 60.0f - (animProgress * 20.0f);  // Drift upward

    // Large "LEVEL UP!" text
    glm::vec3 levelUpColor = PlaceholderAssets::Colors::GOLD_HIGHLIGHT;
    glm::vec3 fadeColor = levelUpColor * alpha;

    textRenderer->renderText("LEVEL UP!",
        START_X - 40.0f, notifY,
        fadeColor, 1.2f);

    // Level number
    std::stringstream ss;
    ss << "Lvl " << levelData_.currentLevel;
    std::string levelStr = ss.str();

    glm::vec3 levelNumColor = glm::vec3(0.2f, 0.8f, 0.2f) * alpha;
    textRenderer->renderText(levelStr,
        START_X - 20.0f, notifY - 20.0f,
        levelNumColor, 0.9f);
}

glm::vec3 UILevelProgress::getProgressColor() const {
    float expPercent = getExperiencePercent();

    if (expPercent < 0.25f) {
        return glm::vec3(0.3f, 0.6f, 1.0f);  // Blue
    } else if (expPercent < 0.5f) {
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green
    } else if (expPercent < 0.75f) {
        return glm::vec3(0.9f, 0.8f, 0.2f);  // Gold
    } else {
        return glm::vec3(1.0f, 0.4f, 0.2f);  // Orange (almost level up)
    }
}

float UILevelProgress::getExperiencePercent() const {
    if (levelData_.experienceNeededForNextLevel <= 0) return 0.0f;

    float percent = static_cast<float>(levelData_.experienceForCurrentLevel) /
                   static_cast<float>(levelData_.experienceNeededForNextLevel);
    return std::max(0.0f, std::min(1.0f, percent));
}

std::string UILevelProgress::formatExperience(int exp) const {
    if (exp >= 1000000) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (exp / 1000000.0f) << "M";
        return ss.str();
    } else if (exp >= 1000) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (exp / 1000.0f) << "K";
        return ss.str();
    } else {
        return std::to_string(exp);
    }
}
