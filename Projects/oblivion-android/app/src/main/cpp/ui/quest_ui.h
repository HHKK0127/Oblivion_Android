#pragma once

#include "../game/quest_manager.h"
#include "../game/npc_manager.h"
#include "../localization/localization_manager.h"
#include <memory>
#include <android/log.h>

#define LOG_TAG "QuestUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

enum class QuestUIPanel {
    QUEST_LOG,       // Current quests list
    QUEST_DETAIL,    // Quest details
    NPC_INTERACTION  // NPC quest dialogue
};

class QuestUI {
private:
    QuestUIPanel currentPanel;
    std::shared_ptr<Quest> selectedQuest;
    bool isVisible;
    int maxVisibleItems;
    int scrollOffset;

    QuestManager* questManager;
    NpcManager* npcManager;
    LocalizationManager* localizationManager;

public:
    QuestUI();
    ~QuestUI();

    void initialize(QuestManager* qm, NpcManager* nm, LocalizationManager* lm);
    void cleanup();

    // Panel transitions
    void showQuestLog();
    void showQuestDetail(uint32_t questId);
    void showNpcQuests(uint32_t npcId);

    // User interaction
    void onTouchEvent(float x, float y);
    void onKeyPress(int key);

    // Rendering
    void render();
    void toggle();
    bool getVisibility() const { return isVisible; }
};
