#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>

/**
 * @brief Level up progress display
 *
 * Phase 21: In-game HUD - experience progress display
 * Displays player current level, experience, and progress to next level
 */
class UILevelProgress {
public:
    struct LevelData {
        int currentLevel;
        int totalExperience;
        int experienceForCurrentLevel;
        int experienceNeededForNextLevel;
        int skillIncreases;  // Skill increases towards level up

        LevelData() : currentLevel(1), totalExperience(0),
                      experienceForCurrentLevel(0), experienceNeededForNextLevel(1000),
                      skillIncreases(0) {}
    };

    UILevelProgress();
    ~UILevelProgress() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    void setLevelData(const LevelData& data) { levelData_ = data; }
    const LevelData& getLevelData() const { return levelData_; }

    void updateExperience(int totalExp, int expInCurrentLevel);
    void updateSkillIncreases(int increases);
    void notifyLevelUp(int newLevel);

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    bool hasLevelUpPending() const { return levelUpPending_; }
    void dismissLevelUp() { levelUpPending_ = false; }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    LevelData levelData_;
    bool levelUpPending_ = false;
    float levelUpAnimationTime_ = 0.0f;

    static constexpr float PROGRESS_BAR_WIDTH = 200.0f;
    static constexpr float PROGRESS_BAR_HEIGHT = 12.0f;
    static constexpr float PANEL_WIDTH = 240.0f;
    static constexpr float PANEL_HEIGHT = 80.0f;
    static constexpr float START_X = 540.0f;  // Center screen
    static constexpr float START_Y = 420.0f;  // Below effects area

    static constexpr float LEVEL_UP_ANIMATION_DURATION = 2.0f;

    void renderProgressPanel();
    void renderExperienceBar();
    void renderLevelInfo();
    void renderLevelUpNotification();
    void renderSkillIncreasesInfo();

    glm::vec3 getProgressColor() const;
    float getExperiencePercent() const;
    std::string formatExperience(int exp) const;
};
