#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief アクティブエフェクト表示
 *
 * Phase 20: ゲーム内HUD - バフ・デバフディスプレイ
 * 現在適用されているバフやデバフを画面に表示
 */
class UIActiveEffects {
public:
    enum EffectType {
        EFFECT_NONE = 0,
        EFFECT_BUFF = 1,
        EFFECT_DEBUFF = 2,
        EFFECT_DISEASE = 3,
        EFFECT_POISON = 4,
        EFFECT_CURSE = 5,
        EFFECT_ENHANCE = 6,
        EFFECT_SPELL = 7
    };

    struct ActiveEffect {
        std::string name;
        EffectType type;
        float duration;        // Remaining duration in seconds
        float maxDuration;     // Original duration
        glm::vec3 color;
        std::string iconChar;  // Single character representation
        int effectId;          // Unique ID for updates

        ActiveEffect() : name(""), type(EFFECT_NONE), duration(0.0f),
                         maxDuration(0.0f), color(1.0f, 1.0f, 1.0f), iconChar("*"),
                         effectId(0) {}
    };

    UIActiveEffects();
    ~UIActiveEffects() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Add/remove effects
    void addEffect(int effectId, const std::string& name, EffectType type,
                   float duration, glm::vec3 color, const std::string& iconChar = "*");

    void removeEffect(int effectId);
    void updateEffectDuration(int effectId, float newDuration);
    void clearAllEffects();

    // Query
    int getEffectCount() const { return effects_.size(); }
    const ActiveEffect* getEffect(int index) const;
    const ActiveEffect* findEffect(int effectId) const;

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    std::vector<std::shared_ptr<ActiveEffect>> effects_;

    static constexpr float ICON_SIZE = 32.0f;
    static constexpr float ICON_GAP = 4.0f;
    static constexpr float PANEL_PADDING = 8.0f;
    static constexpr float START_X = 30.0f;   // Right side
    static constexpr float START_Y = 250.0f;  // Below action prompt

    static constexpr int MAX_VISIBLE_EFFECTS = 12;

    void renderEffectIcons();
    void renderEffectTooltip();
    void renderDurationBars();

    glm::vec3 getEffectColor(EffectType type) const;
    std::string getEffectTypeLabel(EffectType type) const;

    void removeExpiredEffects();
};
