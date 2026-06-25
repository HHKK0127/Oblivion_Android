#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

/**
 * @brief ターゲット情報表示UI
 *
 * Phase 18: ゲーム内HUD - ターゲット情報表示
 * ターゲットした敵やNPCの名前、体力、レベルなどを表示
 */
class UITargetInfo {
public:
    enum TargetType {
        TARGET_NONE = 0,
        TARGET_NPC = 1,
        TARGET_ENEMY = 2,
        TARGET_CREATURE = 3
    };

    struct TargetData {
        std::string name;
        TargetType type;
        float health;
        float maxHealth;
        int level;
        std::string faction;
        bool isHostile;

        TargetData() : name(""), type(TARGET_NONE), health(0.0f),
                       maxHealth(100.0f), level(1), faction(""), isHostile(false) {}
    };

    UITargetInfo();
    ~UITargetInfo() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Set target information
    void setTarget(const TargetData& target);
    void clearTarget();
    bool hasTarget() const { return currentTarget_.type != TARGET_NONE; }

    // Update target health (without full target update)
    void updateTargetHealth(float currentHealth);

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    const TargetData& getTarget() const { return currentTarget_; }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    TargetData currentTarget_;
    float targetLockDuration_ = 0.0f;  // How long target has been locked

    static constexpr float PANEL_WIDTH = 250.0f;
    static constexpr float PANEL_HEIGHT = 100.0f;
    static constexpr float PANEL_X = 30.0f;  // Right side of screen
    static constexpr float PANEL_Y = 100.0f;

    static constexpr float HEALTH_BAR_WIDTH = 200.0f;
    static constexpr float HEALTH_BAR_HEIGHT = 20.0f;
    static constexpr float HEALTH_BAR_PADDING = 10.0f;

    void renderTargetPanel();
    void renderTargetName();
    void renderHealthBar();
    void renderTargetInfo();
    void renderHostileIndicator();

    glm::vec3 getHealthBarColor(float healthPercent) const;
    glm::vec3 getNameColor(TargetType type, bool isHostile) const;

    void renderBar(float x, float y, float currentValue, float maxValue,
                   glm::vec3 barColor, const std::string& label);
};
