#include "ui_quest_log.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UIQuestLog::UIQuestLog(const std::string& title)
    : UIPanel(title) {
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.x,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.y,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.z, 0.97f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::BROWN_ACCENT.x,
        PlaceholderAssets::Colors::BROWN_ACCENT.y,
        PlaceholderAssets::Colors::BROWN_ACCENT.z, 1.0f));
    setBorderWidth(2.0f);
    setTitleBarColor(glm::vec4(
        PlaceholderAssets::Colors::BROWN_ACCENT.x,
        PlaceholderAssets::Colors::BROWN_ACCENT.y,
        PlaceholderAssets::Colors::BROWN_ACCENT.z, 0.95f));
    setTitle(title);
    setCloseButtonVisible(true);
    setDraggable(true);
}

bool UIQuestLog::initialize(QuestManager* qm, TextRenderer* tr) {
    if (!qm || !tr) return false;
    questManager = qm;
    textRenderer = tr;
    if (!UIPanel::initialize()) return false;
    refreshQuests();
    return true;
}

void UIQuestLog::refreshQuests() {
    if (!questManager) return;
    displayedQuests.clear();

    auto active    = questManager->getActiveQuests();
    auto completed = questManager->getCompletedQuests();

    switch (currentFilter) {
        case QuestFilter::ACTIVE:
            displayedQuests = active;
            break;
        case QuestFilter::COMPLETED:
            displayedQuests = completed;
            break;
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
    if (tab >= 0) {
        currentFilter = static_cast<QuestFilter>(tab);
        refreshQuests();
        return true;
    }

    if (hitTestScrollUp(x, y)) {
        if (scrollOffset > 0) --scrollOffset;
        return true;
    }
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

    UIPanel::render();
    renderFilterTabs();
    renderQuestList();
    renderScrollButtons();
    if (selectedQuestIndex >= 0 && selectedQuestIndex < (int)displayedQuests.size())
        renderQuestDetail();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

// ─── filter tabs ──────────────────────────────────────────────────────────────

void UIQuestLog::renderFilterTabs() {
    glm::vec2 cp = getContentPosition();
    const char* labels[] = { "All", "Active", "Completed" };
    const float tabW = 90.0f, tabH = 24.0f, gap = 4.0f;

    for (int i = 0; i < 3; ++i) {
        float bx = cp.x + i * (tabW + gap);
        bool sel = (i == static_cast<int>(currentFilter));

        glm::vec3 bg = sel
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.88f;
        glm::vec3 border = sel
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);

        PlaceholderAssets::drawPanel(bx, cp.y, tabW, tabH, bg, border);
        if (textRenderer)
            textRenderer->renderText(labels[i], bx + 6.0f, cp.y + 4.0f,
                glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
    }
}

// ─── quest list ───────────────────────────────────────────────────────────────

void UIQuestLog::renderQuestList() {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;  // leave 220 px for detail panel
    float y = cp.y + 30.0f;

    int visEnd = std::min(scrollOffset + MAX_VISIBLE, (int)displayedQuests.size());

    for (int i = scrollOffset; i < visEnd; ++i) {
        const auto& q = displayedQuests[i];
        bool sel = (i == selectedQuestIndex);
        float ry = y + (i - scrollOffset) * (ROW_H + ROW_GAP);

        glm::vec3 bg = sel
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.9f;
        glm::vec3 border = sel
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT) * 0.7f;

        PlaceholderAssets::drawPanel(cp.x, ry, listW, ROW_H, bg, border);

        // State colour strip
        PlaceholderAssets::drawSolidRect(cp.x, ry, 5.0f, ROW_H,
            questStateColor(q->state), 1.0f);

        if (textRenderer) {
            textRenderer->renderText(q->title,
                cp.x + 10.0f, ry + 4.0f, glm::vec3(0.1f, 0.1f, 0.1f), 0.8f);

            // Progress: N/M objectives
            uint32_t done = q->getCompletedObjectiveCount();
            uint32_t total = static_cast<uint32_t>(q->objectives.size());
            char buf[32];
            snprintf(buf, sizeof(buf), "%u/%u objectives", done, total);
            textRenderer->renderText(buf,
                cp.x + 10.0f, ry + 22.0f,
                glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.65f);

            // State badge
            textRenderer->renderText(questStateLabel(q->state),
                cp.x + listW - 80.0f, ry + 12.0f,
                questStateColor(q->state), 0.65f);
        }
    }

    if (displayedQuests.empty() && textRenderer) {
        PlaceholderAssets::drawPanel(cp.x, y, listW, 50.0f,
            glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.95f,
            PlaceholderAssets::Colors::BROWN_ACCENT);
        textRenderer->renderText("No quests to display",
            cp.x + 10.0f, y + 16.0f, glm::vec3(0.4f, 0.3f, 0.2f), 0.8f);
    }
}

// ─── detail panel ─────────────────────────────────────────────────────────────

void UIQuestLog::renderQuestDetail() {
    if (selectedQuestIndex < 0 || selectedQuestIndex >= (int)displayedQuests.size()) return;
    const auto& q = displayedQuests[selectedQuestIndex];

    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float detailX = cp.x + cs.x - 215.0f;
    float detailY = cp.y + 30.0f;
    float detailW = 210.0f;
    float detailH = cs.y - 34.0f;

    PlaceholderAssets::drawPanel(detailX, detailY, detailW, detailH,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT),
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));

    if (!textRenderer) return;

    // Title bar
    PlaceholderAssets::drawPanel(detailX, detailY, detailW, 18.0f,
        questStateColor(q->state), PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText(questStateLabel(q->state),
        detailX + 6.0f, detailY + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);

    float tx = detailX + 6.0f;
    float ty = detailY + 22.0f;

    // Quest title
    textRenderer->renderText(q->title, tx, ty, glm::vec3(0.1f, 0.1f, 0.1f), 0.78f);
    ty += 18.0f;

    // Description (truncated)
    if (!q->description.empty()) {
        std::string desc = q->description;
        if (desc.size() > 60) desc = desc.substr(0, 57) + "...";
        textRenderer->renderText(desc, tx, ty,
            glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.6f);
        ty += 24.0f;
    }

    // Divider
    UIDrawHelper::drawColoredQuad(detailX + 4.0f, ty, detailW - 8.0f, 1.0f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT.x, PlaceholderAssets::Colors::BROWN_ACCENT.y, PlaceholderAssets::Colors::BROWN_ACCENT.z, 0.8f),
        screenWidth, screenHeight);
    ty += 6.0f;

    // Objectives
    textRenderer->renderText("Objectives:", tx, ty,
        glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
    ty += 14.0f;

    for (const auto& obj : q->objectives) {
        bool done = obj.isCompleted();
        glm::vec3 col = done
            ? glm::vec3(PlaceholderAssets::Colors::GREEN_STAMINA)
            : glm::vec3(0.3f, 0.3f, 0.3f);
        const char* mark = done ? "[x]" : "[ ]";

        char buf[128];
        snprintf(buf, sizeof(buf), "%s %s", mark, obj.description.c_str());
        textRenderer->renderText(buf, tx, ty, col, 0.58f);
        ty += 12.0f;

        if (obj.targetProgress > 1) {
            snprintf(buf, sizeof(buf), "    %u / %u", obj.currentProgress, obj.targetProgress);
            textRenderer->renderText(buf, tx, ty,
                glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.55f);
            ty += 11.0f;
        }
    }

    // Reward
    if (q->reward.goldAmount > 0 || q->reward.experiencePoints > 0) {
        ty += 6.0f;
        UIDrawHelper::drawColoredQuad(detailX + 4.0f, ty, detailW - 8.0f, 1.0f,
            glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT.x, PlaceholderAssets::Colors::BROWN_ACCENT.y, PlaceholderAssets::Colors::BROWN_ACCENT.z, 0.6f),
            screenWidth, screenHeight);
        ty += 4.0f;
        textRenderer->renderText("Reward:", tx, ty, glm::vec3(0.1f, 0.1f, 0.1f), 0.65f);
        ty += 13.0f;

        if (q->reward.goldAmount > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "  %u gold", q->reward.goldAmount);
            textRenderer->renderText(buf, tx, ty,
                glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT), 0.65f);
            ty += 12.0f;
        }
        if (q->reward.experiencePoints > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "  %.0f XP", q->reward.experiencePoints);
            textRenderer->renderText(buf, tx, ty,
                glm::vec3(PlaceholderAssets::Colors::GREEN_STAMINA), 0.65f);
        }
    }
}

// ─── scroll buttons ───────────────────────────────────────────────────────────

void UIQuestLog::renderScrollButtons() {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float upX = cp.x + listW + 4.0f;
    float upY = cp.y + 30.0f;

    PlaceholderAssets::drawPanel(upX, upY, 20.0f, 20.0f,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer)
        textRenderer->renderText("^", upX + 6.0f, upY + 2.0f,
            glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);

    float dnY = upY + 26.0f;
    PlaceholderAssets::drawPanel(upX, dnY, 20.0f, 20.0f,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer)
        textRenderer->renderText("v", upX + 6.0f, dnY + 2.0f,
            glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UIQuestLog::hitTestFilterTab(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    const float tabW = 90.0f, tabH = 24.0f, gap = 4.0f;
    for (int i = 0; i < 3; ++i) {
        float bx = cp.x + i * (tabW + gap);
        if (x >= bx && x <= bx + tabW && y >= cp.y && y <= cp.y + tabH)
            return i;
    }
    return -1;
}

int UIQuestLog::hitTestQuestRow(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float startY = cp.y + 30.0f;
    for (int i = 0; i < MAX_VISIBLE; ++i) {
        float ry = startY + i * (ROW_H + ROW_GAP);
        if (x >= cp.x && x <= cp.x + listW && y >= ry && y <= ry + ROW_H)
            return i;
    }
    return -1;
}

bool UIQuestLog::hitTestScrollUp(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float upX = cp.x + listW + 4.0f;
    float upY = cp.y + 30.0f;
    return x >= upX && x <= upX + 20.0f && y >= upY && y <= upY + 20.0f;
}

bool UIQuestLog::hitTestScrollDown(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float listW = cs.x - 220.0f;
    float upX = cp.x + listW + 4.0f;
    float dnY = cp.y + 56.0f;
    return x >= upX && x <= upX + 20.0f && y >= dnY && y <= dnY + 20.0f;
}

// ─── helpers ──────────────────────────────────────────────────────────────────

glm::vec3 UIQuestLog::questStateColor(QuestState state) const {
    switch (state) {
        case QuestState::ACCEPTED:    return glm::vec3(0.2f, 0.5f, 0.85f);
        case QuestState::IN_PROGRESS: return glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
        case QuestState::COMPLETED:   return glm::vec3(PlaceholderAssets::Colors::GREEN_STAMINA);
        case QuestState::FAILED:      return glm::vec3(PlaceholderAssets::Colors::RED_HEALTH);
        default:                      return glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK);
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
