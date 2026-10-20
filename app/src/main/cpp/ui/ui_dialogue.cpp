#include "ui_dialogue.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

static const char* kTexSelectionFull   = "dialog__dialog_selection_full";
static const char* kTexSelectionCut    = "dialog__dialog_selection_cut";

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

UIDialogue::UIDialogue(const std::string& title)
    : UIPanel(title.empty() ? "Dialogue" : title) {
    setCloseButtonVisible(true);
    setDraggable(false);
    setVisible(false);
}

bool UIDialogue::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    preloadTextures();
    return UIPanel::initialize();
}

void UIDialogue::preloadTextures() {
    for (int i = 0; i < 9; ++i) loadMenuTex(kBgKeys[i]);
    loadMenuTex(kTexSelectionFull);
    loadMenuTex(kTexSelectionCut);
}

GLuint UIDialogue::loadTex(const std::string& partialPath) {
    auto it = texCache.map.find(partialPath);
    if (it != texCache.map.end()) return it->second;
    std::string full = "textures/ui/" + partialPath;
    if (full.size() < 4 || full.substr(full.size() - 4) != ".png")
        full += ".png";
    GLuint id = TextureLoader::loadTextureFromAsset(full);
    texCache.map[partialPath] = id;
    return id;
}

GLuint UIDialogue::loadMenuTex(const std::string& name) {
    return loadTex("menus/" + name);
}

void UIDialogue::renderGenericBackground(float x, float y, float w, float h, float alpha) {
    GLuint slices[9];
    for (int i = 0; i < 9; ++i)
        slices[i] = texCache.map[std::string("menus/") + kBgKeys[i]];
    UIDrawHelper::drawGenericBackground(slices, x, y, w, h,
                                        screenWidth, screenHeight, alpha);
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

void UIDialogue::selectTopic(int index) {
    if (!currentDialogue) return;
    if (index < 0 || index >= (int)currentDialogue->topics.size()) return;
    currentDialogue->selectTopic(index);
    responseVisible = true;
    responseAlpha = 0.0f;
    if (onTopicSelected)
        onTopicSelected(currentDialogue->topics[index].topicId);
}

void UIDialogue::update(float deltaTime) {
    UIPanel::update(deltaTime);
    if (responseVisible && responseAlpha < 1.0f)
        responseAlpha = std::min(1.0f, responseAlpha + deltaTime * 3.0f);
    else if (!responseVisible && responseAlpha > 0.0f)
        responseAlpha = std::max(0.0f, responseAlpha - deltaTime * 4.0f);
}

bool UIDialogue::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;
    if (isInsideCloseButton(x, y)) { closeDialogue(); return true; }
    if (!currentDialogue) return false;
    int row = hitTestTopicRow(x, y);
    if (row >= 0) {
        int topicIdx = scrollOffset + row;
        if (topicIdx < (int)currentDialogue->topics.size()) {
            currentDialogue->selectTopic(topicIdx);
            responseVisible = true;
            responseAlpha = 0.0f;
            if (onTopicSelected)
                onTopicSelected(currentDialogue->topics[topicIdx].topicId);
            return true;
        }
    }
    return true;
}

void UIDialogue::render() {
    if (!isVisible() || !currentDialogue) return;
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    renderGenericBackground(cp.x, cp.y, cs.x, cs.y);
    renderNPCPortrait();
    renderGreeting();
    renderResponse();
    renderTopicList();
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIDialogue::renderNPCPortrait() {
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float ph = pw * 1.2f;
    GLuint selTex = texCache.map[std::string("menus/") + kTexSelectionFull];
    if (selTex != 0) {
        UIDrawHelper::drawTexturedQuad(cp.x, cp.y, pw, ph, selTex,
                                       glm::vec4(1.0f), screenWidth, screenHeight);
    }
    if (textRenderer && currentDialogue) {
        textRenderer->renderText(currentDialogue->npcName,
            cp.x, cp.y + ph + 4.0f,
            glm::vec3(0.85f, 0.65f, 0.13f), 0.75f);
    }
}

void UIDialogue::renderGreeting() {
    if (!textRenderer || !currentDialogue) return;
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float tx = cp.x + pw + 10.0f;
    float ty = getGreetingY();
    float maxW = getContentSize().x - pw - 14.0f;
    GLuint cutTex = texCache.map[std::string("menus/") + kTexSelectionCut];
    if (cutTex != 0) {
        UIDrawHelper::drawTexturedQuad(tx, ty - 4.0f, maxW, 56.0f, cutTex,
                                       glm::vec4(1.0f), screenWidth, screenHeight);
    }
    textRenderer->renderText(currentDialogue->greeting, tx + 6.0f, ty,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.72f);
}

void UIDialogue::renderResponse() {
    if (!responseVisible || responseAlpha <= 0.0f || !textRenderer || !currentDialogue)
        return;
    DialogueTopic* topic = currentDialogue->getSelectedTopic();
    if (!topic || topic->responseText.empty()) return;
    glm::vec2 cp = getContentPosition();
    float pw = getPortraitW();
    float tx = cp.x + pw + 10.0f;
    float ty = getResponseY();
    float maxW = getContentSize().x - pw - 14.0f;
    GLuint selTex = texCache.map[std::string("menus/") + kTexSelectionFull];
    if (selTex != 0) {
        UIDrawHelper::drawTexturedQuad(tx, ty - 4.0f, maxW, 70.0f, selTex,
                                       glm::vec4(1.0f, 1.0f, 1.0f, responseAlpha * 0.85f),
                                       screenWidth, screenHeight);
    }
    textRenderer->renderText(topic->responseText, tx + 6.0f, ty,
        glm::vec3(0.15f, 0.1f, 0.08f), 0.68f);
}

void UIDialogue::renderTopicList() {
    if (!textRenderer || !currentDialogue) return;
    glm::vec2 cp = getContentPosition();
    float ty = getTopicListY();
    float rowW = getContentSize().x;

    {
        GLuint selTex = texCache.map[std::string("menus/") + kTexSelectionCut];
        if (selTex != 0) {
            UIDrawHelper::drawTexturedQuad(cp.x, ty, rowW, 18.0f, selTex,
                                           glm::vec4(1.0f), screenWidth, screenHeight);
        }
        textRenderer->renderText("Topics", cp.x + 6.0f, ty + 2.0f,
            glm::vec3(0.85f, 0.65f, 0.13f), 0.65f);
    }
    ty += 20.0f;

    int count = (int)currentDialogue->topics.size();
    int visEnd = std::min(scrollOffset + MAX_TOPICS, count);

    for (int i = scrollOffset; i < visEnd; ++i) {
        const DialogueTopic& topic = currentDialogue->topics[i];
        bool sel = (i == currentDialogue->selectedTopicIndex);
        float ry = ty + (float)(i - scrollOffset) * (TOPIC_ROW_H + TOPIC_ROW_GAP);

        GLuint tex = texCache.map[std::string("menus/") +
            (sel ? kTexSelectionFull : kTexSelectionCut)];
        if (tex != 0) {
            glm::vec4 tint = sel
                ? glm::vec4(0.55f, 0.45f, 0.33f, 1.0f)
                : glm::vec4(0.68f, 0.58f, 0.48f, 0.85f);
            UIDrawHelper::drawTexturedQuad(cp.x, ry, rowW, TOPIC_ROW_H, tex, tint,
                                           screenWidth, screenHeight);
        }

        if (topic.isQuest) {
            UIDrawHelper::drawColoredQuad(cp.x, ry, 5.0f, TOPIC_ROW_H,
                                          glm::vec4(0.85f, 0.65f, 0.13f, 1.0f),
                                          screenWidth, screenHeight);
        }

        textRenderer->renderText(topic.topicText,
            cp.x + 52.0f, ry + 8.0f,
            sel ? glm::vec3(0.85f, 0.65f, 0.13f) : glm::vec3(0.1f, 0.1f, 0.1f),
            0.75f);
    }

    float byY = ty + (float)MAX_TOPICS * (TOPIC_ROW_H + TOPIC_ROW_GAP) + 6.0f;
    {
        GLuint selTex = texCache.map[std::string("menus/") + kTexSelectionCut];
        if (selTex != 0) {
            UIDrawHelper::drawTexturedQuad(cp.x, byY, 120.0f, 30.0f, selTex,
                                           glm::vec4(0.55f, 0.45f, 0.33f, 1.0f),
                                           screenWidth, screenHeight);
        }
    }
    textRenderer->renderText("Goodbye", cp.x + 6.0f, byY + 6.0f,
        glm::vec3(0.85f, 0.85f, 0.85f), 0.75f);
}

int UIDialogue::hitTestTopicRow(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float rowW = getContentSize().x;
    float ty = getTopicListY() + 20.0f;
    for (int i = 0; i < MAX_TOPICS; ++i) {
        float ry = ty + (float)i * (TOPIC_ROW_H + TOPIC_ROW_GAP);
        if (x >= cp.x && x <= cp.x + rowW && y >= ry && y <= ry + TOPIC_ROW_H)
            return i;
    }
    return -1;
}

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
    return cp.y + pw * 1.2f + 16.0f + 14.0f;
}