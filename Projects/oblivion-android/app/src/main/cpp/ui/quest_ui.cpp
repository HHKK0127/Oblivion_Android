#include "quest_ui.h"

QuestUI::QuestUI()
    : currentPanel(QuestUIPanel::QUEST_LOG), isVisible(false),
      maxVisibleItems(5), scrollOffset(0),
      questManager(nullptr), npcManager(nullptr), localizationManager(nullptr) {
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

    // Simple touch handling - check if user tapped close button area
    // In a real implementation, would handle menu selections
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
    if (!isVisible) return;

    std::string panelTitle;
    switch (currentPanel) {
        case QuestUIPanel::QUEST_LOG: {
            panelTitle = localizationManager ?
                localizationManager->getString("quest_log") : "Quest Log";
            LOGD("=== %s ===", panelTitle.c_str());

            if (questManager) {
                auto activeQuests = questManager->getActiveQuests();
                for (size_t i = 0; i < activeQuests.size(); ++i) {
                    if (i >= scrollOffset && i < scrollOffset + maxVisibleItems) {
                        auto quest = activeQuests[i];
                        LOGD("  [%zu] %s (Objectives: %u/%zu)",
                             i + 1, quest->title.c_str(),
                             quest->getCompletedObjectiveCount(),
                             quest->objectives.size());
                    }
                }
            }
            break;
        }

        case QuestUIPanel::QUEST_DETAIL: {
            panelTitle = localizationManager ?
                localizationManager->getString("quest_detail") : "Quest Details";
            LOGD("=== %s ===", panelTitle.c_str());

            if (selectedQuest) {
                LOGD("Title: %s", selectedQuest->title.c_str());
                LOGD("Description: %s", selectedQuest->description.c_str());
                LOGD("Status: %d", static_cast<int>(selectedQuest->state));
                LOGD("Objectives:");
                for (const auto& obj : selectedQuest->objectives) {
                    LOGD("  - %s (%u/%u)", obj.description.c_str(),
                         obj.currentProgress, obj.targetProgress);
                }
                LOGD("Reward: Gold=%u, Exp=%.1f",
                     selectedQuest->reward.goldAmount,
                     selectedQuest->reward.experiencePoints);
            }
            break;
        }

        case QuestUIPanel::NPC_INTERACTION: {
            panelTitle = localizationManager ?
                localizationManager->getString("npc_available_quests") : "Available Quests";
            LOGD("=== %s ===", panelTitle.c_str());

            // Placeholder for NPC quests display
            LOGD("(NPC quest list would be displayed here)");
            break;
        }

        default:
            break;
    }
}

void QuestUI::toggle() {
    isVisible = !isVisible;
    LOGD("Quest UI toggled: %s", isVisible ? "visible" : "hidden");
}
