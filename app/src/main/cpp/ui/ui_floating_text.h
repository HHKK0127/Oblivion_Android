#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief フローティングテキストシステム
 *
 * Phase 17: ゲーム内HUD - 浮遊テキスト表示
 * ダメージ、ヒール、ステータス効果などを画面上に浮かぶテキストで表示
 */
class UIFloatingText {
public:
    enum TextType {
        DAMAGE = 0,
        HEAL = 1,
        CRITICAL = 2,
        MISS = 3,
        BLOCK = 4,
        BUFF = 5,
        DEBUFF = 6,
        LEVEL_UP = 7
    };

    struct FloatingTextInstance {
        std::string text;
        glm::vec3 position;      // World or screen position
        TextType type;
        float duration;          // Total lifetime in seconds
        float elapsedTime;       // Current lifetime
        float velocity_y;        // Upward movement per second
        float startAlpha;

        FloatingTextInstance(const std::string& txt, glm::vec3 pos, TextType t, float dur)
            : text(txt), position(pos), type(t), duration(dur), elapsedTime(0.0f),
              velocity_y(-30.0f), startAlpha(1.0f) {}
    };

    UIFloatingText();
    ~UIFloatingText() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Add floating text at screen position
    void addText(const std::string& text, float screenX, float screenY,
                 TextType type, float duration = 2.0f);

    // Add floating text at world position (requires projection)
    void addWorldText(const std::string& text, glm::vec3 worldPos,
                      TextType type, float duration = 2.0f);

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    void clear() { instances_.clear(); }
    int getInstanceCount() const { return instances_.size(); }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    std::vector<std::shared_ptr<FloatingTextInstance>> instances_;

    static constexpr float MAX_VISIBLE_DISTANCE = 200.0f;  // Screen distance
    static constexpr int MAX_INSTANCES = 32;

    glm::vec3 getTextColor(TextType type) const;
    float getTextScale(TextType type) const;
    float getCurrentAlpha(const FloatingTextInstance& instance) const;

    void removeExpiredInstances();
};
