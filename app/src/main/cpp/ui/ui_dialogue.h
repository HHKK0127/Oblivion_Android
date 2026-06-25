#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/dialogue.h"
#include <memory>
#include <functional>

/**
 * @brief NPCダイアログUI
 *
 * Phase 10: Oblivion風のNPCとの会話画面。
 * NPCポートレート領域、挨拶テキスト、会話トピックリスト、
 * NPCの返答テキストを羊皮紙パネルで表示。
 */
class UIDialogue : public UIPanel {
public:
    using TopicCallback = std::function<void(const std::string& topicId)>;

    explicit UIDialogue(const std::string& title = "");
    ~UIDialogue() override = default;

    bool initialize(TextRenderer* textRenderer);

    // Open dialogue with NPC
    void openDialogue(std::shared_ptr<Dialogue> dialogue);
    void closeDialogue();

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    // Callback when player selects a topic
    void setOnTopicSelected(TopicCallback cb) { onTopicSelected = cb; }

private:
    TextRenderer* textRenderer = nullptr;
    std::shared_ptr<Dialogue> currentDialogue;
    TopicCallback onTopicSelected;

    int hoveredTopicIndex = -1;
    int scrollOffset = 0;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    // Animation: response text fade-in
    float responseAlpha = 0.0f;
    bool  responseVisible = false;

    static constexpr float TOPIC_ROW_H   = 36.0f;
    static constexpr float TOPIC_ROW_GAP = 3.0f;
    static constexpr int   MAX_TOPICS    = 6;

    void renderNPCPortrait();
    void renderGreeting();
    void renderResponse();
    void renderTopicList();

    int  hitTestTopicRow(float x, float y) const;

    // Layout helpers
    float getPortraitW() const;
    float getGreetingY() const;
    float getResponseY() const;
    float getTopicListY() const;
};
