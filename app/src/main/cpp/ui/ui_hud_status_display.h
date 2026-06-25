#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <memory>

struct CharacterStatus;

/**
 * @brief HUDステータス表示
 *
 * Phase 15: ゲーム内HUD - 画面左上のプレイヤーステータス表示
 * HP、MP、スタミナバー及びレベル・経験値情報を画面上に常時表示
 */
class UIHudStatusDisplay {
public:
    UIHudStatusDisplay();
    ~UIHudStatusDisplay() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    void setCharacter(CharacterStatus* character) { character_ = character; }

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

private:
    TextRenderer* textRenderer = nullptr;
    CharacterStatus* character_ = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    static constexpr float BAR_WIDTH = 150.0f;
    static constexpr float BAR_HEIGHT = 12.0f;
    static constexpr float BAR_GAP = 4.0f;
    static constexpr float START_X = 10.0f;
    static constexpr float START_Y = 10.0f;

    void renderHealthBar(float x, float y);
    void renderManaBar(float x, float y);
    void renderStaminaBar(float x, float y);
    void renderLevelInfo(float x, float y);

    glm::vec3 getHealthColor(float healthPercent) const;
    glm::vec3 getManaColor(float manaPercent) const;
    glm::vec3 getStaminaColor(float staminaPercent) const;

    void renderBar(float x, float y, float currentValue, float maxValue,
                   glm::vec3 barColor, const std::string& label);
};
