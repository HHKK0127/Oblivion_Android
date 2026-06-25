#include "ui_dialogue.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UIDialogue::UIDialogue(const std::string& title)
    : UIPanel(title.empty() ? "Dialogue" : title) {
    // Dark parchment — more dramatic than inventory/quest panels
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_DARK.r * 0.7f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.g * 0.6f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.b * 0.5f, 0.97f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.r,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.g,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.b, 1.0f));
    setBorderWidth(3.0f);
    setTitleBarColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));  // hidden — portrait takes its place
    setCloseButtonVisible(true);
    setDraggable(false);  // Oblivion dialogue is fixed
}

bool UIDialogue::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    return UIPanel::initialize();
}

void UIDialogue::openDialogue(std::shared_ptr<Dialogue> dlg) {
    currentDialogue = dlg;
    if (currentDialogue) {
        currentDialogue->isActive = true;
        currentDialogue->selectedTopicIndex = -1;
    }
    scrollOffset = 0;
    hoveredTopicIndex = -1;
    responseVisible = false;
    responseAlpha = 0.0f;
    setVisible(true);
}

void UIDialogue::closeDialogue() {
    if (currentDialogue) currentDialogue->end();
    currentDialogue = nullptr;
    setVisible(false);
}

void UIDialogue::update(float deltaTime) {
    UIPanel::update(deltaTime);

    // Fade-in animation for response text
    if (responseVisible && responseAlpha < 1.0f) {
        responseAlpha = std::min(1.0f, responseAlpha + deltaTime * 3.0f);
    } else if (!responseVisible && responseAlpha > 0.0f) {
        responseAlpha = std::max(0.0f, responseAlpha - deltaTime * 4.0f);
    }
}

bool UIDialogue::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    if (isInsideCloseButton(x, y)) {
        closeDialogue();
        return true;
    }

    if (!currentDialogue) return false;

    // Topic row tap
    int row = hitTestTopicRow(x, y);
    if (row >= 0) {
        int topicIdx = scrollOffset + row;
        if (topicIdx < (int)currentDialogue->topics.size()) {
            currentDialogue->selectTopic(topicIdx);
            responseVisible = true;
            responseAlpha   = 0.0f;  // start fade-in
            if (onTopicSelected)
                onTopicSelected(currentDialogue->topics[topicIdx].topicId);
            return true;
        }
    }

    return true;  // consume all touches while open
}

void UIDialogue::render() {
    if (!isVisible() || !currentDialogue) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderNPCPortrait();
    renderGreeting();
    renderResponse();
    renderTopicList();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

// ─── rendering sections ────────────────────────────────────────────────────────

void UIDialogue::renderNPCPortrait() {
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float ph = pw * 1.2f;  // tall-ish portrait

    // Portrait frame — gold bordered dark background
    PlaceholderAssets::drawPanel(cp.x, cp.y, pw, ph,
        PlaceholderAssets::Colors::DARK_GRAY,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);

    // Placeholder silhouette (brown square inside)
    float pad = pw * 0.1f;
    PlaceholderAssets::drawSolidRect(cp.x + pad, cp.y + pad, pw - pad * 2.0f, ph - pad * 2.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT, 0.6f);

    // NPC name below portrait
    if (textRenderer && currentDialogue) {
        textRenderer->renderText(currentDialogue->npcName,
            cp.x, cp.y + ph + 4.0f,
            glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT), 0.75f);
    }
}

void UIDialogue::renderGreeting() {
    if (!textRenderer || !currentDialogue) return;
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float tx = cp.x + pw + 10.0f;
    float ty = getGreetingY();
    float maxW = getContentSize().x - pw - 14.0f;

    // Greeting area background
    PlaceholderAssets::drawPanel(tx, ty - 4.0f, maxW, 56.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f,
        PlaceholderAssets::Colors::BROWN_ACCENT);

    // Greeting text (truncate to two lines)
    std::string greet = currentDialogue->greeting;
    textRenderer->renderText(greet, tx + 6.0f, ty,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.72f);
}

void UIDialogue::renderResponse() {
    if (!responseVisible || responseAlpha <= 0.0f) return;
    if (!textRenderer || !currentDialogue) return;

    auto* topic = currentDialogue->getSelectedTopic();
    if (!topic || topic->responseText.empty()) return;

    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float tx = cp.x + pw + 10.0f;
    float ty = getResponseY();
    float maxW = getContentSize().x - pw - 14.0f;

    // Response area — slightly different shade
    glm::vec4 bgCol(
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.r * 0.8f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.g * 0.78f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.b * 0.65f,
        responseAlpha * 0.9f);
    UIDrawHelper::drawColoredQuad(tx, ty - 4.0f, maxW, 70.0f, bgCol,
        screenWidth, screenHeight);
    UIDrawHelper::drawBorder(tx, ty - 4.0f, maxW, 70.0f, 1.5f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT, responseAlpha),
        screenWidth, screenHeight);

    textRenderer->renderText(topic->responseText, tx + 6.0f, ty,
        glm::vec3(0.15f, 0.1f, 0.08f) * (1.0f / responseAlpha),
        0.68f);
}

void UIDialogue::renderTopicList() {
    if (!textRenderer || !currentDialogue) return;
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float tx = cp.x;
    float ty = getTopicListY();
    float rowW = getContentSize().x;

    // Topics header
    PlaceholderAssets::drawPanel(tx, ty, rowW, 18.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("Topics",
        tx + 6.0f, ty + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);
    ty += 20.0f;

    int count = static_cast<int>(currentDialogue->topics.size());
    int visEnd = std::min(scrollOffset + MAX_TOPICS, count);

    for (int i = scrollOffset; i < visEnd; ++i) {
        const auto& topic = currentDialogue->topics[i];
        bool sel = (i == currentDialogue->selectedTopicIndex);

        glm::vec3 bg = sel
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.88f;
        glm::vec3 border = sel
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT) * 0.7f;

        float ry = ty + (i - scrollOffset) * (TOPIC_ROW_H + TOPIC_ROW_GAP);
        PlaceholderAssets::drawPanel(tx, ry, rowW, TOPIC_ROW_H, bg, border);

        // Quest topic indicator
        if (topic.isQuest) {
            PlaceholderAssets::drawSolidRect(tx, ry, 5.0f, TOPIC_ROW_H,
                glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT), 1.0f);
        }

        textRenderer->renderText(topic.topicText,
            tx + 10.0f, ry + 10.0f,
            sel ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT)
                : glm::vec3(0.1f, 0.1f, 0.1f),
            0.75f);
    }

    // Goodbye button at bottom
    float byY = ty + MAX_TOPICS * (TOPIC_ROW_H + TOPIC_ROW_GAP) + 6.0f;
    PlaceholderAssets::drawPanel(tx, byY, 120.0f, 30.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("Goodbye",
        tx + 6.0f, byY + 6.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.75f);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UIDialogue::hitTestTopicRow(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float rowW = getContentSize().x;
    float ty = getTopicListY() + 20.0f;  // below header

    for (int i = 0; i < MAX_TOPICS; ++i) {
        float ry = ty + i * (TOPIC_ROW_H + TOPIC_ROW_GAP);
        if (x >= cp.x && x <= cp.x + rowW && y >= ry && y <= ry + TOPIC_ROW_H)
            return i;
    }
    return -1;
}

// ─── layout helpers ───────────────────────────────────────────────────────────

float UIDialogue::getPortraitW() const {
    return std::min(120.0f, getContentSize().x * 0.28f);
}

float UIDialogue::getGreetingY() const {
    return getContentPosition().y + 6.0f;
}

float UIDialogue::getResponseY() const {
    return getContentPosition().y + 70.0f;
}

float UIDialogue::getTopicListY() const {
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    return cp.y + pw * 1.2f + 16.0f + 14.0f;  // below portrait + name
}
