#pragma once

#include <memory>
#include <chrono>
#include <android/log.h>
#include "../ui/title_screen.h"
#include "../ui/quest_ui.h"
#include "../game/quest_manager.h"
#include "../game/combat_manager.h"
#include "../game/spell_manager.h"
#include "../game/world_manager.h"
#include "../localization/localization_manager.h"
#include "../profiling/performance_monitor.h"
#include "../save_system/save_manager.h"

#define LOG_TAG "Renderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class Renderer {
private:
    // UI Systems
    std::unique_ptr<TitleScreen> titleScreen;
    std::unique_ptr<QuestUI> questUI;

    // Game Systems
    std::unique_ptr<LocalizationManager> localizationManager;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<QuestManager> questManager;
    std::unique_ptr<CombatManager> combatManager;
    std::unique_ptr<SpellManager> spellManager;

    // Profiling
    std::unique_ptr<PerformanceMonitor> performanceMonitor;

    // Save System
    std::unique_ptr<SaveManager> saveManager;

    // State
    bool showTitleScreen;
    unsigned int screenWidth;
    unsigned int screenHeight;

    // Frame Rate Control
    int targetFPS;
    float frameTimeThreshold;  // Milliseconds per frame (1000/fps)
    std::chrono::high_resolution_clock::time_point lastFrameTime;

public:
    Renderer();
    ~Renderer();

    bool init(unsigned int width, unsigned int height);
    void render(float deltaTime);
    void cleanup();
    void resize(unsigned int width, unsigned int height);

    // Getters
    LocalizationManager* getLocalizationManager() { return localizationManager.get(); }
    QuestManager* getQuestManager() { return questManager.get(); }
    CombatManager* getCombatManager() { return combatManager.get(); }
    SpellManager* getSpellManager() { return spellManager.get(); }
    WorldManager* getWorldManager() { return worldManager.get(); }
    TitleScreen* getTitleScreen() { return titleScreen.get(); }
    QuestUI* getQuestUI() { return questUI.get(); }
    PerformanceMonitor* getPerformanceMonitor() { return performanceMonitor.get(); }

    bool isTitleScreenActive() const { return showTitleScreen; }

    // FPS Control
    void setTargetFPS(int fps);
    int getTargetFPS() const { return targetFPS; }

    // Save/Load
    bool saveGameState(const std::string& slotName);
    bool loadGameState(const std::string& slotName);
    SaveManager* getSaveManager() { return saveManager.get(); }

private:
    void initLocalization();
    void initGameSystems();
    void createTestScenario();
};
