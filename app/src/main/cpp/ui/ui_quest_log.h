#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/quest_manager.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <GLES3/gl3.h>

class UIQuestLog : public UIPanel {
public:
    enum class QuestFilter { ALL = 0, ACTIVE = 1, COMPLETED = 2 };

    explicit UIQuestLog(const std::string& title = "Quest Log");
    ~UIQuestLog() override = default;

    bool initialize(QuestManager* questMgr, TextRenderer* textRenderer);

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }
    void refreshQuests();

private:
    QuestManager* questManager = nullptr;
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    QuestFilter currentFilter = QuestFilter::ACTIVE;
    int selectedQuestIndex = -1;
    int scrollOffset = 0;

    std::vector<std::shared_ptr<Quest>> displayedQuests;

    static constexpr float ROW_H = 52.0f;
    static constexpr float ROW_GAP = 4.0f;
    static constexpr int MAX_VISIBLE = 7;

    // p68: texture cache
    std::unordered_map<std::string, GLuint> texCache;
    GLuint loadTex(const std::string& partialPath);
    GLuint loadMenuTex(const std::string& name);
    GLuint tex(const std::string& key) const;
    void preloadTextures();
    void renderGenericBackground(float x, float y, float w, float h, float alpha = 1.0f);

    void renderFilterTabs();
    void renderQuestList();
    void renderQuestDetail();
    void renderScrollButtons();

    int  hitTestFilterTab(float x, float y) const;
    int  hitTestQuestRow(float x, float y) const;
    bool hitTestScrollUp(float x, float y) const;
    bool hitTestScrollDown(float x, float y) const;

    glm::vec3 questStateColor(QuestState state) const;
    const char* questStateLabel(QuestState state) const;
};