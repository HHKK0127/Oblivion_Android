#pragma once

#include "dialogue_record.h"
#include "dialogue_tree.h"
#include "dialogue_filter_engine.h"
#include "dialogue_runner.h"
#include "dialogue_history.h"
#include "../game/npc_manager.h"
#include "../game/quest_manager.h"
#include "../game/faction_manager.h"
#include "../game/player.h"
#include "../script/script_manager.h"
#include "../assets/esm_reader.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <android/log.h>

#define DI_LOG_TAG "DialogueIntegration"
#ifdef ENABLE_DEBUG_LOGS
#define DI_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DI_LOG_TAG, __VA_ARGS__)
#else
#define DI_LOGD(...) do {} while(0)
#endif
#define DI_LOGI(...) __android_log_print(ANDROID_LOG_INFO, DI_LOG_TAG, __VA_ARGS__)
#define DI_LOGW(...) __android_log_print(ANDROID_LOG_WARN, DI_LOG_TAG, __VA_ARGS__)
#define DI_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DI_LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace dialogue {

// ============================================================================
// DialogueIntegration - Unified dialogue system coordinator
// ============================================================================
class DialogueIntegration {
public:
    // Callback for UI events
    using UICallback = std::function<void(const DialogueEvent&)>;

    DialogueIntegration();
    ~DialogueIntegration();

    // Initialize with game systems
    bool initialize(
        NpcManager* npcMgr,
        QuestManager* questMgr,
        oblivion::FactionManager* factionMgr,
        oblivion::script::ScriptManager* scriptMgr,
        Player* player
    );

    // Load dialogue data from ESM
    void loadFromESM(const oblivion::ESMManager& esmMgr);

    // Set UI callback
    void setUICallback(UICallback cb) { uiCallback = cb; }

    // Start dialogue with nearest NPC
    bool startDialogueWithNPC(uint32_t npcFormID);

    // Start dialogue with NPC by proximity
    bool startDialogueWithNearestNPC(const glm::vec3& playerPos, float maxDistance = 3.0f);

    // End current dialogue
    void endDialogue();

    // Player selects a topic
    bool selectTopic(int topicIndex);

    // Player selects a choice
    bool selectChoice(int choiceIndex);

    // Process text input
    bool processTextInput(const std::string& input);

    // Update (call per frame)
    void update(float deltaTime);

    // Check if dialogue is active
    bool isDialogueActive() const;

    // Get current NPC FormID
    uint32_t getCurrentNPCFormID() const { return runner.getCurrentNPCFormID(); }

    // Get dialogue runner
    const DialogueRunner& getRunner() const { return runner; }
    DialogueRunner& getRunner() { return runner; }

    // Get dialogue history
    const DialogueHistory& getHistory() const { return history; }

    // Get filter engine
    const DialogueFilterEngine& getFilterEngine() const { return filterEngine; }

    // Get dialogue tree for NPC
    DialogueTree* getTreeForNPC(uint32_t npcFormID);

    // Check if NPC has dialogue
    bool hasDialogue(uint32_t npcFormID) const;

    // Get all loaded NPC FormIDs with dialogue
    std::vector<uint32_t> getAllDialogueNPCs() const;

    // Statistics
    size_t getLoadedTreeCount() const { return trees.size(); }
    size_t getTotalNodeCount() const;

    // Logging
    void logStats() const;

private:
    // Game system references
    NpcManager* npcManager = nullptr;
    QuestManager* questManager = nullptr;
    oblivion::FactionManager* factionManager = nullptr;
    oblivion::script::ScriptManager* scriptManager = nullptr;
    Player* player = nullptr;

    // Dialogue subsystems
    DialogueFilterEngine filterEngine;
    DialogueRunner runner;
    DialogueHistory history;

    // Dialogue trees (NPC FormID -> tree)
    std::unordered_map<uint32_t, std::unique_ptr<DialogueTree>> trees;

    // UI callback
    UICallback uiCallback;

    // Proximity check state
    uint32_t lastProximityCheckNPC = 0;
    float proximityCheckTimer = 0.0f;
    static constexpr float PROXIMITY_CHECK_INTERVAL = 0.5f;

    // Helper: build tree from ESM data
    void buildTreeForNPC(uint32_t npcFormID, const std::vector<DialogueDialRecord>& dialRecords);

    // Helper: update filter engine context
    void updateFilterContext(uint32_t npcFormID);

    // Helper: handle dialogue events
    void handleDialogueEvent(const DialogueEvent& event);

    // Helper: find nearest NPC with dialogue
    uint32_t findNearestDialogueNPC(const glm::vec3& playerPos, float maxDistance) const;
};

} // namespace dialogue
} // namespace oblivion
