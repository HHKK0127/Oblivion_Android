#include "ui_quest_log.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

static const char* kBgKeys[9] = {
    "genericbackground__center_background",
    "genericbackground__edge_top",
    "genericbackground__edge_bottom",
    "genericbackground__edge_left",
    "genericbackground__edge_right",
    "genericbackground__top_left_corner",
    "genericbackground__top_right_corner",
    "genericbackground__bottom_left_corner",
    "genericbackground__bottom_right_corner",
};

UIQuestLog::UIQuestLog(const std::string& title)
    : UIPanel(title) {
    setCloseButtonVisible(true);
    setDraggable(true);
    setVisible(false);
}

bool UIQuestLog::initialize(QuestManager* qm, TextRenderer* tr) {
    if (!qm || !tr) return false;
    questManager = qm;
    textRenderer = tr;
    preloadTextures();
    if (!UIPanel::initialize()) return false;
    refreshQuests();
    return true;
}

void UIQuestLog::preloadTextures() {
    for (int i = 0; i < 9; ++i) loadMenuTex(kBgKeys[i]);
}

GLuint UIQuestLog::loadTex(const std::string& partialPath) {
    auto it = texCache.find(partialPath);
    if (it != texCache.end()) return it->second;
    std::string full = "textures/ui/" + partialPath;
    if (full.size() < 4 || full.substr(full.size() - 4) != ".png")
        full += ".png";
    GLuint id = TextureLoader::loadTextureFromAsset(full);
    texCache[partialPath] = id;
    return id;
}

GLuint UIQuestLog::loadMenuTex(const std::string& name) {
    return loadTex("menus/" + name);
}

GLuint UIQuestLog::tex(const std::string& key) const {
    auto it = texCache.find(key);
    return (it != texCache.end()) ? it->second : 0;
}

void UIQuestLog::renderGenericBackground(float x, float y, float w, float h, float alpha) {
    GLuint slices[9];
    for (int i = 0; i < 9; ++i)
        slices[i] = tex(std::string("menus/") + kBgKeys[i]);
    UIDrawHelper::drawGenericBackground(slices, x, y, w, h,
                                        screenWidth, screenHeight, alpha);
}

void UIQuestLog::refreshQuests() {
    if (!questManager) return;
    displayedQuests.clear();
    auto active = questManager->getActiveQuests();
    auto completed = questManager->getCompletedQuests();
    switch (currentFilter) {
        case QuestFilter::ACTIVE:    displayedQuests = active; break;
        case QuestFilter::COMPLETED: displayedQuests = completed; break;
        case QuestFilter::ALL:
            displayedQuests = active;
            displayedQuests.insert(displayedQuests.end(), completed.begin(), completed.end());
            break;
    }
    selectedQuestIndex = -1;
    scrollOffset = 0;
}

void UIQuestLog::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UIQuestLog::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;
    if (isInsideCloseButton(x, y) || isInsideTitleBar(x, y))
        return UIPanel::onTouchDown(x, y, pointerId);
    int tab = hitTestFilterTab(x, y);
    if (tab >= 0) { currentFilter = (QuestFilter)tab; refreshQuests(); return true; }
    if (hitTestScrollUp(x, y)) { if (scrollOffset > 0) --scrollOffset; return true; }
    if (hitTestScrollDown(x, y)) {
        int maxOff = std::max(0, (int)displayedQuests.size() - MAX_VISIBLE);
        if (scrollOffset < maxOff) ++scrollOffset;
        return true;
    }
    int row = hitTestQuestRow(x, y);
    if (row >= 0 && scrollOffset + row < (int)displayedQuests.size()) {
        selectedQuestIndex = scrollOffset + row;
        return true;
    }
    return UIPanel::onTouchDown(x, y, pointerId);
}

void UIQuestLog::render() {
    if (!isVisible()) return;
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    renderGenericBackground(cp.x, cp.y, cs.x, cs.y);
    renderFilterTabs();
    renderQuestList();
    renderScrollButtons();
    if (selectedQuestIndex >= 0 && selectedQuestIndex < (int)displayedQuests.size())
        renderQuestDetail();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIQuestLog::renderFilterTabs() {
    glm::vec2 cp = getContentPosition();
    const char* labels[] = { "All", "Active", "Completed" };
    const float tabW = 90.0f, tabH = 24.0f, gap = 4.0f;
    for (int i = 0; i < 3; ++i) {
        float bx = cp.x + i * (tabW + gap);
        bool sel = (i == (int)currentFilter);
        GLuint t = tex(std::string("inventory/") +
            (sel ? "inv_icon_tab_all" : "inv_icon_tab_misc"));
        if (t != 0) {
            UIDrawHelper::drawTexturedQuad(bx, cp.y, tabW, tabH, t,
                                           glm::vec4(1.0f), screenWidth, screenHeight);
        }
        if (textRenderer)
            textRenderer->renderText(labels[i], bx + 6.0f, cp.y + 4.0f,
                                     glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
    }
}

void UIQuestLog::renderQuestList() {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float y = cp.y + 30.0f;
    int visEnd = std::min(scrollOffset + MAX_VISIBLE, (int)displayedQuests.size());
    for (int i = scrollOffset; i < visEnd; ++i) {
        auto& q = displayedQuests[i];
        bool sel = (i == selectedQuestIndex);
        float ry = y + (float)(i - scrollOffset) * (ROW_H + ROW_GAP);

        // Selection texture background
        GLuint t = tex(std::string("menus/") +
            (sel ? "dialog__dialog_selection_full" : "dialog__dialog_selection_cut"));
        if (t != 0) {
            glm::vec4 tint = sel
                ? glm::vec4(0.55f, 0.45f, 0.33f, 1.0f)
                : glm::vec4(0.68f, 0.58f, 0.48f, 0.85f);
            UIDrawHelper::drawTexturedQuad(cp.x, ry, listW, ROW_H, t, tint,
                                           screenWidth, screenHeight);
        }

        // State color strip (left bar)
        glm::vec3 sc = questStateColor(q->state);
        UIDrawHelper::drawColoredQuad(cp.x, ry, 5.0f, ROW_H,
                                      glm::vec4(sc.x, sc.y, sc.z, 1.0f),
                                      screenWidth, screenHeight);

        if (textRenderer) {
            textRenderer->renderText(q->title, cp.x + 10.0f, ry + 4.0f,
                                     glm::vec3(0.1f, 0.1f, 0.1f), 0.8f);
            uint32_t done = q->getCompletedObjectiveCount();
            uint32_t total = (uint32_t)q->objectives.size();
            char buf[32];
            snprintf(buf, sizeof(buf), "%u/%u objectives", done, total);
            textRenderer->renderText(buf, cp.x + 10.0f, ry + 22.0f,
                                     glm::vec3(0.36f, 0.25f, 0.20f), 0.65f);
            textRenderer->renderText(questStateLabel(q->state),
                                     cp.x + listW - 80.0f, ry + 12.0f,
                                     sc, 0.65f);
        }
    }
    if (displayedQuests.empty() && textRenderer) {
        GLuint t = tex("menus/dialog__dialog_selection_cut");
        if (t != 0) {
            UIDrawHelper::drawTexturedQuad(cp.x, y, listW, 50.0f, t,
                                           glm::vec4(0.65f, 0.55f, 0.45f, 0.85f),
                                           screenWidth, screenHeight);
        }
        textRenderer->renderText("No quests to display",
            cp.x + 10.0f, y + 16.0f, glm::vec3(0.4f, 0.3f, 0.2f), 0.8f);
    }
}

void UIQuestLog::renderQuestDetail() {
    if (selectedQuestIndex < 0 || selectedQuestIndex >= (int)displayedQuests.size()) return;
    auto& q = displayedQuests[selectedQuestIndex];
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float detailX = cp.x + cs.x - 215.0f;
    float detailY = cp.y + 30.0f;
    float detailW = 210.0f;
    float detailH = cs.y - 34.0f;

    // Detail panel background
    GLuint t = tex("menus/dialog__dialog_selection_cut");
    if (t != 0) {
        UIDrawHelper::drawTexturedQuad(detailX, detailY, detailW, detailH, t,
                                       glm::vec4(0.7f, 0.6f, 0.5f, 0.95f),
                                       screenWidth, screenHeight);
    }

    if (!textRenderer) return;

    // Title bar (colored by state)
    glm::vec3 sc = questStateColor(q->state);
    UIDrawHelper::drawColoredQuad(detailX, detailY, detailW, 18.0f,
                                  glm::vec4(sc.x, sc.y, sc.z, 0.85f),
                                  screenWidth, screenHeight);
    textRenderer->renderText(questStateLabel(q->state),
        detailX + 6.0f, detailY + 2.0f,
        glm::vec3(0.77f, 0.66f, 0.50f), 0.65f);

    float tx = detailX + 6.0f;
    float ty = detailY + 22.0f;

    textRenderer->renderText(q->title, tx, ty, glm::vec3(0.1f, 0.1f, 0.1f), 0.78f);
    ty += 18.0f;

    if (!q->description.empty()) {
        std::string desc = q->description;
        if (desc.size() > 60) desc = desc.substr(0, 57) + "...";
        textRenderer->renderText(desc, tx, ty, glm::vec3(0.36f, 0.25f, 0.20f), 0.6f);
        ty += 24.0f;
    }

    // Divider
    UIDrawHelper::drawColoredQuad(detailX + 4.0f, ty, detailW - 8.0f, 1.0f,
                                  glm::vec4(0.36f, 0.25f, 0.20f, 0.8f),
                                  screenWidth, screenHeight);
    ty += 6.0f;

    textRenderer->renderText("Objectives:", tx, ty, glm::vec3(0.1f, 0.1f, 0.1f), 0.7);
    ty += 14.0f;

    for (const auto& obj : q->objectives) {
        bool done = obj.isCompleted();
        glm::vec3 col = done
            ? glm::vec3(0.4f, 0.7f, 0.3f)
            : glm::vec3(0.3f, 0.3f, 0.3f);
        const char* mark = done ? "[x]" : "[ ]";
        char buf[128];
        snprintf(buf, sizeof(buf), "%s %s", mark, obj.description.c_str());
        textRenderer->renderText(buf, tx, ty, col, 0.58f);
        ty += 12.0f;
        if (obj.targetProgress > 1) {
            snprintf(buf, sizeof(buf), "    %u / %u", obj.currentProgress, obj.targetProgress);
            textRenderer->renderText(buf, tx, ty, glm::vec3(0.36f, 0.25f, 0.20f), 0.55f);
            ty += 11.0f;
        }
    }

    if (q->reward.goldAmount > 0 || q->reward.experiencePoints > 0) {
        ty += 6.0f;
        UIDrawHelper::drawColoredQuad(detailX + 4.0f, ty, detailW - 8.0f, 1.0f,
                                      glm::vec4(0.36f, 0.25f, 0.20f, 0.6f),
                                      screenWidth, screenHeight);
        ty += 4.0f;
        textRenderer->renderText("Reward:", tx, ty, glm::vec3(0.1f, 0.1f, 0.1f), 0.65f);
        ty += 13.0f;
        if (q->reward.goldAmount > 0) {
            char buf2[32];
            snprintf(buf2, sizeof(buf2), "  %u gold", q->reward.goldAmount);
            textRenderer->renderText(buf2, tx, ty, glm::vec3(0.85f, 0.65f, 0.13f), 0.65f);
            ty += 12.0f;
        }
        if (q->reward.experiencePoints > 0) {
            char buf2[32];
            snprintf(buf2, sizeof(buf2), "  %.0f XP", q->reward.experiencePoints);
            textRenderer->renderText(buf2, tx, ty, glm::vec3(0.4f, 0.7f, 0.3f), 0.65f);
        }
    }
}

void UIQuestLog::renderScrollButtons() {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float upX = cp.x + listW + 4.0f;
    float upY = cp.y + 30.0f;
    float bSize = 20.0f;

    GLuint t = tex("menus/dialog__dialog_selection_cut");
    if (t != 0) {
        UIDrawHelper::drawTexturedQuad(upX, upY, bSize, bSize, t,
                                       glm::vec4(0.55f, 0.45f, 0.33f, 1.0f),
                                       screenWidth, screenHeight);
        UIDrawHelper::drawTexturedQuad(upX, upY + bSize + 6.0f, bSize, bSize, t,
                                       glm::vec4(0.55f, 0.45f, 0.33f, 1.0f),
                                       screenWidth, screenHeight);
    }
    if (textRenderer) {
        textRenderer->renderText("^", upX + 6.0f, upY + 2.0f,
                                 glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
        textRenderer->renderText("v", upX + 6.0f, upY + bSize + 8.0f,
                                 glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
    }
}

int UIQuestLog::hitTestFilterTab(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    const float tabW = 90.0f, tabH = 24.0f, gap = 4.0f;
    for (int i = 0; i < 3; ++i) {
        float bx = cp.x + i * (tabW + gap);
        if (x >= bx && x <= bx + tabW && y >= cp.y && y <= cp.y + tabH) return i;
    }
    return -1;
}

int UIQuestLog::hitTestQuestRow(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float startY = cp.y + 30.0f;
    for (int i = 0; i < MAX_VISIBLE; ++i) {
        float ry = startY + (float)i * (ROW_H + ROW_GAP);
        if (x >= cp.x && x <= cp.x + listW && y >= ry && y <= ry + ROW_H) return i;
    }
    return -1;
}

bool UIQuestLog::hitTestScrollUp(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float upX = cp.x + listW + 4.0f;
    return x >= upX && x <= upX + 20.0f && y >= cp.y + 30.0f && y <= cp.y + 50.0f;
}

bool UIQuestLog::hitTestScrollDown(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float upX = cp.x + listW + 4.0f;
    float dnY = cp.y + 56.0f;
    return x >= upX && x <= upX + 20.0f && y >= dnY && y <= dnY + 20.0f;
}

glm::vec3 UIQuestLog::questStateColor(QuestState state) const {
    switch (state) {
        case QuestState::ACCEPTED:    return glm::vec3(0.2f, 0.5f, 0.85f);
        case QuestState::IN_PROGRESS: return glm::vec3(0.85f, 0.65f, 0.13f);
        case QuestState::COMPLETED:   return glm::vec3(0.4f, 0.7f, 0.3f);
        case QuestState::FAILED:      return glm::vec3(0.8f, 0.1f, 0.1f);
        default:                      return glm::vec3(0.55f, 0.45f, 0.33f);
    }
}

const char* UIQuestLog::questStateLabel(QuestState state) const {
    switch (state) {
        case QuestState::PENDING:     return "Pending";
        case QuestState::ACCEPTED:    return "Accepted";
        case QuestState::IN_PROGRESS: return "In Progress";
        case QuestState::COMPLETED:   return "Completed";
        case QuestState::FAILED:      return "Failed";
        default:                      return "Unknown";
    }
}