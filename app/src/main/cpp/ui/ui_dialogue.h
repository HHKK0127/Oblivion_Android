#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/dialogue.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <GLES3/gl3.h>

/**
 * @brief NPC dialogue UI
 *
 * Phase 10 + p68: Oblivion-style NPC conversation screen with
 * BSA-original textures.
 */
class UIDialogue : public UIPanel {
public:
    using TopicCallback = std::function<void(const std::string& topicId)>;

    explicit UIDialogue(const std::string& title = "");
    ~UIDialogue() override = default;

        using UIPanel::initialize;
        bool initialize(TextRenderer* textRenderer);

    // Open dialogue with NPC
    void openDialogue(std::shared_ptr<Dialogue> dialogue);
    void closeDialogue();

    // Programmatic topic selection
    void selectTopic(int index);

    bool hasDialogue() const { return currentDialogue != nullptr; }

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    void setOnTopicSelected(TopicCallback cb) { onTopicSelected = cb; }

private:
    TextRenderer* textRenderer = nullptr;
    std::shared_ptr<Dialogue> currentDialogue;
    TopicCallback onTopicSelected;

    int hoveredTopicIndex = -1;
    int scrollOffset = 0;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    float responseAlpha = 0.0f;
    bool  responseVisible = false;

    static constexpr float TOPIC_ROW_H   = 36.0f;
    static constexpr float TOPIC_ROW_GAP = 3.0f;
    static constexpr int   MAX_TOPICS    = 6;

    // ── Texture management (p68) ──
    struct TextureCache {
        std::unordered_map<std::string, GLuint> map;
        GLuint get(const std::string& key) const;
    };
    TextureCache texCache;

    void preloadTextures();
    GLuint loadTex(const std::string& partialPath);
    GLuint loadMenuTex(const std::string& name);  // "menus/" + name + ".png"
    GLuint loadInventoryTex(const std::string& name); // "inventory/" + name + ".png"

    // ── Rendering ──
    void renderNPCPortrait();
    void renderGreeting();
    void renderResponse();
    void renderTopicList();
    void renderGenericBackground(float x, float y, float w, float h, float alpha = 1.0f);

    int  hitTestTopicRow(float x, float y) const;

    // Layout helpers
    float getPortraitW() const;
    float getGreetingY() const;
    float getResponseY() const;
    float getTopicListY() const;
};
