#pragma once

#include "dialogue_tree.h"
#include "dialogue_filter_engine.h"
#include "dialogue_history.h"
#include "../script/script_manager.h"
#include "../game/player.h"
#include "../game/npc_manager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <android/log.h>

#define DR_LOG_TAG "DialogueRunner"
#ifdef ENABLE_DEBUG_LOGS
#define DR_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DR_LOG_TAG, __VA_ARGS__)
#else
#define DR_LOGD(...) do {} while(0)
#endif
#define DR_LOGI(...) __android_log_print(ANDROID_LOG_INFO, DR_LOG_TAG, __VA_ARGS__)
#define DR_LOGW(...) __android_log_print(ANDROID_LOG_WARN, DR_LOG_TAG, __VA_ARGS__)
#define DR_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DR_LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace dialogue {

// ============================================================================
// Dialogue state
// ============================================================================
enum class DialogueState : uint8_t {
    Idle,           // No dialogue active
    Greeting,       // NPC greeting phase
    TopicSelection, // Player selecting topic
    Response,       // NPC responding
    Choice,         // Player making a choice
    Farewell,       // Dialogue ending
    ScriptExecution // Running result script
};

// ============================================================================
// Dialogue choice (for branching)
// ============================================================================
struct DialogueChoice {
    std::string text;           // Choice display text
    uint32_t targetNodeID = 0;  // Node to go to if selected
    uint32_t topicFormID = 0;   // Topic to navigate to
};

// ============================================================================
// Dialogue event (for UI)
// ============================================================================
struct DialogueEvent {
    enum class Type : uint8_t {
        Greeting,
        Response,
        TopicList,
        ChoiceList,
        Farewell,
        ScriptResult,
        SkillCheck,
        Error
    };

    Type type;
    std::string text;
    std::vector<std::string> options;
    std::vector<DialogueChoice> choices;
    uint32_t speakerFormID = 0;
    bool success = true;
};

// ============================================================================
// DialogueRunner - Manages dialogue flow and player interaction
// ============================================================================
class DialogueRunner {
public:
    // Callback for dialogue events (UI integration)
    using EventCallback = std::function<void(const DialogueEvent&)>;

    DialogueRunner();
    ~DialogueRunner();

    // Set game system references
    void setFilterEngine(DialogueFilterEngine* engine) { filterEngine = engine; }
    void setHistory(DialogueHistory* hist) { history = hist; }
    void setScriptManager(oblivion::script::ScriptManager* sm) { scriptManager = sm; }
    void setNpcManager(NpcManager* nm) { npcManager = nm; }
    void setPlayer(Player* p) { player = p; }

    // Set event callback
    void setEventCallback(EventCallback cb) { eventCallback = cb; }

    // Start dialogue with an NPC
    bool startDialogue(uint32_t npcFormID, DialogueTree& tree);

    // End current dialogue
    void endDialogue();

    // Select a topic by index
    bool selectTopic(int topicIndex);

    // Select a topic by FormID
    bool selectTopicByFormID(uint32_t topicFormID);

    // Select a choice (for branching dialogue)
    bool selectChoice(int choiceIndex);

    // Process player text input (for text-based dialogue)
    bool processTextInput(const std::string& input);

    // Get current state
    DialogueState getState() const { return state; }

    // Get current NPC FormID
    uint32_t getCurrentNPCFormID() const { return currentNPCFormID; }

    // Get current dialogue tree
    DialogueTree* getCurrentTree() const { return currentTree; }

    // Get current response text
    std::string getCurrentResponse() const { return currentResponse; }

    // Get available topics for current dialogue
    std::vector<std::string> getAvailableTopicNames() const;

    // Get available topic FormIDs
    std::vector<uint32_t> getAvailableTopicFormIDs() const;

    // Get current choices
    const std::vector<DialogueChoice>& getCurrentChoices() const { return currentChoices; }

    // Check if dialogue is active
    bool isDialogueActive() const { return state != DialogueState::Idle; }

    // Update (for script execution timing)
    void update(float deltaTime);

    // Statistics
    size_t getTopicsDiscussed() const { return topicsDiscussed; }

private:
    DialogueFilterEngine* filterEngine = nullptr;
    DialogueHistory* history = nullptr;
    oblivion::script::ScriptManager* scriptManager = nullptr;
    NpcManager* npcManager = nullptr;
    Player* player = nullptr;
    EventCallback eventCallback;

    // Current dialogue state
    DialogueState state = DialogueState::Idle;
    uint32_t currentNPCFormID = 0;
    DialogueTree* currentTree = nullptr;

    // Current response
    std::string currentResponse;
    uint32_t currentNodeID = 0;

    // Available topics
    std::vector<uint32_t> availableTopicFormIDs;
    std::vector<std::string> availableTopicNames;

    // Current choices
    std::vector<DialogueChoice> currentChoices;

    // Script execution
    bool scriptRunning = false;
    float scriptTimer = 0.0f;
    static constexpr float SCRIPT_TIMEOUT = 5.0f;

    // Statistics
    size_t topicsDiscussed = 0;

    // Helper methods
    void emitEvent(const DialogueEvent& event);
    void updateAvailableTopics();
    void executeResultScript(uint32_t scriptFormID);
    void applySpeechcraftBonus(float amount);
    bool matchTopicFromText(const std::string& input) const;
    void transitionToState(DialogueState newState);
};

} // namespace dialogue
} // namespace oblivion
