#include "quest_ui.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include <sstream>
#include <iomanip>

QuestUI::QuestUI()
    : currentPanel(QuestUIPanel::QUEST_LOG), isVisible(false),
      maxVisibleItems(5), scrollOffset(0),
      questManager(nullptr), npcManager(nullptr), localizationManager(nullptr),
      textRenderer(nullptr), screenWidth(1080.0f), screenHeight(1920.0f) {
    LOGD("QuestUI created");
}

QuestUI::~QuestUI() {
    cleanup();
    LOGD("QuestUI destroyed");
}

void QuestUI::initialize(QuestManager* qm, NpcManager* nm, LocalizationManager* lm) {
    questManager = qm;
    npcManager = nm;
    localizationManager = lm;

    currentPanel = QuestUIPanel::QUEST_LOG;
    isVisible = false;
    scrollOffset = 0;

    LOGI("QuestUI initialized");
}

void QuestUI::cleanup() {
    questManager = nullptr;
    npcManager = nullptr;
    localizationManager = nullptr;
    LOGD("QuestUI cleaned up");
}

void QuestUI::showQuestLog() {
    currentPanel = QuestUIPanel::QUEST_LOG;
    isVisible = true;
    scrollOffset = 0;
    LOGD("Showing quest log");
}

void QuestUI::showQuestDetail(uint32_t questId) {
    if (questManager) {
        selectedQuest = questManager->getQuest(questId);
        currentPanel = QuestUIPanel::QUEST_DETAIL;
        isVisible = true;
        LOGD("Showing quest detail for quest %u", questId);
    }
}

void QuestUI::showNpcQuests(uint32_t npcId) {
    currentPanel = QuestUIPanel::NPC_INTERACTION;
    isVisible = true;
    LOGD("Showing NPC quests for NPC %u", npcId);
}

void QuestUI::onTouchEvent(float x, float y) {
    if (!isVisible) return;

    float panelW = screenWidth - PANEL_MARGIN * 2;
    float panelH = (currentPanel == QuestUIPanel::QUEST_DETAIL) ?
        screenHeight * 0.75f : screenHeight * 0.7f;
    float panelX = PANEL_MARGIN;
    float panelY = (screenHeight - panelH) / 2.0f;

    // Close button hit test
    float closeX = panelX + panelW - 45.0f;
    float closeY = panelY + 5.0f;
    if (isPointInRect(x, y, closeX, closeY, 40.0f, 40.0f)) {
        isVisible = false;
        LOGD("Quest UI closed via close button");
        return;
    }

    // Back button hit test (detail panel)
    if (currentPanel == QuestUIPanel::QUEST_DETAIL) {
        float backX = panelX + 10.0f;
        float backY = panelY + panelH - BUTTON_HEIGHT - 10.0f;
        if (isPointInRect(x, y, backX, backY, 100.0f, BUTTON_HEIGHT)) {
            currentPanel = QuestUIPanel::QUEST_LOG;
            LOGD("Quest UI: back to quest log");
            return;
        }
    }

    // Quest list item hit test
    if (currentPanel == QuestUIPanel::QUEST_LOG && questManager) {
        float contentX = panelX + 15.0f;
        float contentY = panelY + TITLE_HEIGHT + 10.0f;
        float contentW = panelW - 30.0f;

        auto activeQuests = questManager->getActiveQuests();
        int startIdx = scrollOffset;
        int endIdx = std::min(startIdx + maxVisibleItems, (int)activeQuests.size());

        for (int i = startIdx; i < endIdx; ++i) {
            float itemY = contentY + (i - startIdx) * ITEM_HEIGHT;
            if (isPointInRect(x, y, contentX, itemY, contentW, ITEM_HEIGHT - 2.0f)) {
                showQuestDetail(activeQuests[i]->questId);
                LOGD("Selected quest: %s", activeQuests[i]->title.c_str());
                return;
            }
        }

        // Scroll buttons
        float scrollY = contentY + maxVisibleItems * ITEM_HEIGHT + 5.0f;
        if (isPointInRect(x, y, contentX, scrollY, 60.0f, 40.0f)) {
            // Up
            if (scrollOffset > 0) scrollOffset--;
            LOGD("Quest list scroll up: %d", scrollOffset);
            return;
        }
        if (isPointInRect(x, y, contentX + 70.0f, scrollY, 60.0f, 40.0f)) {
            // Down
            int maxScroll = std::max(0, (int)activeQuests.size() - maxVisibleItems);
            if (scrollOffset < maxScroll) scrollOffset++;
            LOGD("Quest list scroll down: %d", scrollOffset);
            return;
        }
    }

    LOGD("Quest UI touch event: (%.1f, %.1f)", x, y);
}

void QuestUI::onKeyPress(int key) {
    if (!isVisible) return;

    if (key == 4) {  // Back key
        isVisible = false;
        LOGD("Quest UI closed via back key");
    }
}

void QuestUI::render() {
    if (!isVisible || !textRenderer) return;

    switch (currentPanel) {
        case QuestUIPanel::QUEST_LOG:
            renderQuestLog();
            break;
        case QuestUIPanel::QUEST_DETAIL:
            renderQuestDetail();
            break;
        case QuestUIPanel::NPC_INTERACTION:
            renderNpcInteraction();
            break;
    }
}

bool QuestUI::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) const {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

void QuestUI::renderCloseButton(float x, float y, float w, float h) {
    UIDrawHelper::drawColoredQuad(x, y, w, h,
        glm::vec4(0.7f, 0.2f, 0.2f, PANEL_ALPHA), screenWidth, screenHeight);
    if (textRenderer) {
        textRenderer->renderText("X", x + w * 0.35f, y + h * 0.65f,
            glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
    }
}

void QuestUI::renderScrollButtons(float x, float y, float w) {
    float btnW = 60.0f;
    float btnH = 40.0f;

    // Up button
    UIDrawHelper::drawColoredQuad(x, y, btnW, btnH,
        glm::vec4(0.3f, 0.3f, 0.4f, PANEL_ALPHA), screenWidth, screenHeight);
    if (textRenderer) {
        textRenderer->renderText("UP", x + 10.0f, y + btnH * 0.65f,
            glm::vec3(1.0f, 1.0f, 0.8f), 0.8f);
    }

    // Down button
    UIDrawHelper::drawColoredQuad(x + btnW + 10.0f, y, btnW, btnH,
        glm::vec4(0.3f, 0.3f, 0.4f, PANEL_ALPHA), screenWidth, screenHeight);
    if (textRenderer) {
        textRenderer->renderText("DN", x + btnW + 20.0f, y + btnH * 0.65f,
            glm::vec3(1.0f, 1.0f, 0.8f), 0.8f);
    }
}

void QuestUI::renderQuestLog() {
    float panelW = screenWidth - PANEL_MARGIN * 2;
    float panelH = screenHeight * 0.7f;
    float panelX = PANEL_MARGIN;
    float panelY = (screenHeight - panelH) / 2.0f;

    // Background panel
    UIDrawHelper::drawColoredQuad(panelX, panelY, panelW, panelH,
        glm::vec4(0.08f, 0.06f, 0.12f, PANEL_ALPHA), screenWidth, screenHeight);

    // Border
    UIDrawHelper::drawBorder(panelX, panelY, panelW, panelH,
        2.0f, glm::vec4(0.6f, 0.5f, 0.3f, 1.0f), (int)screenWidth, (int)screenHeight);

    // Title bar
    UIDrawHelper::drawColoredQuad(panelX, panelY, panelW, TITLE_HEIGHT,
        glm::vec4(0.15f, 0.12f, 0.08f, 1.0f), screenWidth, screenHeight);

    std::string title = localizationManager ?
        localizationManager->getString("quest_log") : "Quest Log";
    textRenderer->renderText(title.c_str(), panelX + 15.0f, panelY + TITLE_HEIGHT * 0.7f,
        glm::vec3(1.0f, 0.85f, 0.4f), 1.2f);

    // Close button
    renderCloseButton(panelX + panelW - 45.0f, panelY + 5.0f, 40.0f, 40.0f);

    // Quest list
    float contentY = panelY + TITLE_HEIGHT + 10.0f;
    float contentX = panelX + 15.0f;
    float contentW = panelW - 30.0f;

    if (questManager) {
        auto activeQuests = questManager->getActiveQuests();

        if (activeQuests.empty()) {
            textRenderer->renderText("No active quests", contentX, contentY + 30.0f,
                glm::vec3(0.6f, 0.6f, 0.6f), 1.0f);
        } else {
            int startIdx = scrollOffset;
            int endIdx = std::min(startIdx + maxVisibleItems, (int)activeQuests.size());

            for (int i = startIdx; i < endIdx; ++i) {
                auto quest = activeQuests[i];
                float itemY = contentY + (i - startIdx) * ITEM_HEIGHT;

                // Quest item background
                glm::vec4 itemBg = (i % 2 == 0) ?
                    glm::vec4(0.12f, 0.10f, 0.08f, 0.6f) :
                    glm::vec4(0.10f, 0.08f, 0.06f, 0.6f);
                UIDrawHelper::drawColoredQuad(contentX, itemY, contentW, ITEM_HEIGHT - 2.0f,
                    itemBg, screenWidth, screenHeight);

                // Quest state indicator
                glm::vec3 stateColor;
                std::string stateStr;
                switch (quest->state) {
                    case QuestState::IN_PROGRESS:
                        stateColor = glm::vec3(0.4f, 0.8f, 0.4f);
                        stateStr = "[ACTIVE]";
                        break;
                    case QuestState::ACCEPTED:
                        stateColor = glm::vec3(0.4f, 0.6f, 0.9f);
                        stateStr = "[ACCEPT]";
                        break;
                    case QuestState::COMPLETED:
                        stateColor = glm::vec3(0.8f, 0.8f, 0.2f);
                        stateStr = "[DONE]";
                        break;
                    case QuestState::FAILED:
                        stateColor = glm::vec3(0.8f, 0.3f, 0.3f);
                        stateStr = "[FAILED]";
                        break;
                    default:
                        stateColor = glm::vec3(0.5f, 0.5f, 0.5f);
                        stateStr = "[PEND]";
                        break;
                }
                textRenderer->renderText(stateStr.c_str(), contentX + 5.0f, itemY + 28.0f,
                    stateColor, 0.7f);

                // Quest title
                textRenderer->renderText(quest->title.c_str(), contentX + 80.0f, itemY + 28.0f,
                    glm::vec3(0.9f, 0.85f, 0.7f), 0.9f);

                // Objective progress
                std::stringstream objSS;
                objSS << "Obj: " << quest->getCompletedObjectiveCount()
                      << "/" << quest->objectives.size();
                textRenderer->renderText(objSS.str().c_str(), contentX + contentW - 100.0f, itemY + 28.0f,
                    glm::vec3(0.6f, 0.7f, 0.8f), 0.7f);
            }

            // Scroll buttons
            if (activeQuests.size() > maxVisibleItems) {
                renderScrollButtons(contentX, contentY + maxVisibleItems * ITEM_HEIGHT + 5.0f, contentW);
            }
        }
    } else {
        textRenderer->renderText("Quest system not available", contentX, contentY + 30.0f,
            glm::vec3(0.6f, 0.3f, 0.3f), 1.0f);
    }
}

void QuestUI::renderQuestDetail() {
    float panelW = screenWidth - PANEL_MARGIN * 2;
    float panelH = screenHeight * 0.75f;
    float panelX = PANEL_MARGIN;
    float panelY = (screenHeight - panelH) / 2.0f;

    // Background panel
    UIDrawHelper::drawColoredQuad(panelX, panelY, panelW, panelH,
        glm::vec4(0.08f, 0.06f, 0.12f, PANEL_ALPHA), screenWidth, screenHeight);

    // Border
    UIDrawHelper::drawBorder(panelX, panelY, panelW, panelH,
        2.0f, glm::vec4(0.6f, 0.5f, 0.3f, 1.0f), (int)screenWidth, (int)screenHeight);

    // Title bar
    UIDrawHelper::drawColoredQuad(panelX, panelY, panelW, TITLE_HEIGHT,
        glm::vec4(0.15f, 0.12f, 0.08f, 1.0f), screenWidth, screenHeight);

    std::string title = localizationManager ?
        localizationManager->getString("quest_detail") : "Quest Details";
    textRenderer->renderText(title.c_str(), panelX + 15.0f, panelY + TITLE_HEIGHT * 0.7f,
        glm::vec3(1.0f, 0.85f, 0.4f), 1.2f);

    // Close button
    renderCloseButton(panelX + panelW - 45.0f, panelY + 5.0f, 40.0f, 40.0f);

    // Back button
    UIDrawHelper::drawColoredQuad(panelX + 10.0f, panelY + panelH - BUTTON_HEIGHT - 10.0f,
        100.0f, BUTTON_HEIGHT,
        glm::vec4(0.3f, 0.3f, 0.4f, PANEL_ALPHA), screenWidth, screenHeight);
    textRenderer->renderText("Back", panelX + 25.0f, panelY + panelH - BUTTON_HEIGHT + 15.0f,
        glm::vec3(1.0f, 1.0f, 0.8f), 1.0f);

    if (!selectedQuest) {
        textRenderer->renderText("No quest selected", panelX + 20.0f, panelY + TITLE_HEIGHT + 40.0f,
            glm::vec3(0.6f, 0.6f, 0.6f), 1.0f);
        return;
    }

    float contentX = panelX + 20.0f;
    float contentY = panelY + TITLE_HEIGHT + 15.0f;
    float lineH = 30.0f;

    // Quest title
    textRenderer->renderText(selectedQuest->title.c_str(), contentX, contentY + lineH,
        glm::vec3(1.0f, 0.9f, 0.5f), 1.3f);
    contentY += lineH * 1.8f;

    // Quest state
    glm::vec3 stateColor;
    std::string stateStr;
    switch (selectedQuest->state) {
        case QuestState::IN_PROGRESS:
            stateColor = glm::vec3(0.4f, 0.8f, 0.4f);
            stateStr = "Status: In Progress";
            break;
        case QuestState::ACCEPTED:
            stateColor = glm::vec3(0.4f, 0.6f, 0.9f);
            stateStr = "Status: Accepted";
            break;
        case QuestState::COMPLETED:
            stateColor = glm::vec3(0.8f, 0.8f, 0.2f);
            stateStr = "Status: Completed";
            break;
        case QuestState::FAILED:
            stateColor = glm::vec3(0.8f, 0.3f, 0.3f);
            stateStr = "Status: Failed";
            break;
        default:
            stateColor = glm::vec3(0.5f, 0.5f, 0.5f);
            stateStr = "Status: Pending";
            break;
    }
    textRenderer->renderText(stateStr.c_str(), contentX, contentY + lineH,
        stateColor, 0.9f);
    contentY += lineH * 1.5f;

    // Description
    textRenderer->renderText("Description:", contentX, contentY + lineH,
        glm::vec3(0.8f, 0.7f, 0.5f), 0.9f);
    contentY += lineH;
    textRenderer->renderText(selectedQuest->description.c_str(), contentX + 10.0f, contentY + lineH,
        glm::vec3(0.7f, 0.7f, 0.7f), 0.85f);
    contentY += lineH * 2.0f;

    // Objectives
    textRenderer->renderText("Objectives:", contentX, contentY + lineH,
        glm::vec3(0.8f, 0.7f, 0.5f), 0.9f);
    contentY += lineH;

    for (const auto& obj : selectedQuest->objectives) {
        glm::vec3 objColor = (obj.state == QuestObjectiveState::COMPLETED) ?
            glm::vec3(0.4f, 0.8f, 0.4f) : glm::vec3(0.7f, 0.7f, 0.7f);

        std::stringstream objSS;
        objSS << "- " << obj.description << " (" << obj.currentProgress << "/" << obj.targetProgress << ")";
        textRenderer->renderText(objSS.str().c_str(), contentX + 10.0f, contentY + lineH,
            objColor, 0.8f);
        contentY += lineH;
    }

    contentY += lineH * 0.5f;

    // Rewards
    textRenderer->renderText("Rewards:", contentX, contentY + lineH,
        glm::vec3(0.8f, 0.7f, 0.5f), 0.9f);
    contentY += lineH;

    std::stringstream rewardSS;
    rewardSS << "Gold: " << selectedQuest->reward.goldAmount
             << "  EXP: " << std::fixed << std::setprecision(0) << selectedQuest->reward.experiencePoints;
    textRenderer->renderText(rewardSS.str().c_str(), contentX + 10.0f, contentY + lineH,
        glm::vec3(1.0f, 0.85f, 0.4f), 0.85f);
}

void QuestUI::renderNpcInteraction() {
    float panelW = screenWidth - PANEL_MARGIN * 2;
    float panelH = screenHeight * 0.6f;
    float panelX = PANEL_MARGIN;
    float panelY = (screenHeight - panelH) / 2.0f;

    // Background panel
    UIDrawHelper::drawColoredQuad(panelX, panelY, panelW, panelH,
        glm::vec4(0.08f, 0.06f, 0.12f, PANEL_ALPHA), screenWidth, screenHeight);

    // Border
    UIDrawHelper::drawBorder(panelX, panelY, panelW, panelH,
        2.0f, glm::vec4(0.6f, 0.5f, 0.3f, 1.0f), (int)screenWidth, (int)screenHeight);

    // Title bar
    UIDrawHelper::drawColoredQuad(panelX, panelY, panelW, TITLE_HEIGHT,
        glm::vec4(0.15f, 0.12f, 0.08f, 1.0f), screenWidth, screenHeight);

    std::string title = localizationManager ?
        localizationManager->getString("npc_available_quests") : "Available Quests";
    textRenderer->renderText(title.c_str(), panelX + 15.0f, panelY + TITLE_HEIGHT * 0.7f,
        glm::vec3(1.0f, 0.85f, 0.4f), 1.2f);

    // Close button
    renderCloseButton(panelX + panelW - 45.0f, panelY + 5.0f, 40.0f, 40.0f);

    float contentX = panelX + 20.0f;
    float contentY = panelY + TITLE_HEIGHT + 15.0f;

    // Placeholder content
    textRenderer->renderText("NPC quest list", contentX, contentY + 30.0f,
        glm::vec3(0.7f, 0.7f, 0.7f), 1.0f);
    textRenderer->renderText("Talk to NPCs to receive quests", contentX, contentY + 60.0f,
        glm::vec3(0.5f, 0.5f, 0.5f), 0.9f);
}

void QuestUI::toggle() {
    isVisible = !isVisible;
    LOGD("Quest UI toggled: %s", isVisible ? "visible" : "hidden");
}
