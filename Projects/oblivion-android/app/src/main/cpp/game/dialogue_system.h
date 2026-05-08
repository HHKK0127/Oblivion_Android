#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <android/log.h>

#define LOG_TAG "DialogueSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations
class NPC;

// ============================================================
// DialogueNode: Single conversation node with options
// ============================================================
struct DialogueNode {
    std::string nodeId;                              // Unique node identifier
    std::string npcSpeakKey;                        // Localization key for NPC dialogue
    std::vector<std::string> playerOptionKeys;      // Localization keys for player options
    std::unordered_map<int, std::string> nextNodes; // Option index -> next node ID
    std::unordered_map<int, uint32_t> questRewards; // Option index -> quest reward
    std::unordered_map<int, int> goldRewards;       // Option index -> gold reward
    bool endsDialogue = false;                       // Does this node end conversation

    DialogueNode() = default;
};

// ============================================================
// DialogueTree: Complete dialogue tree for an NPC
// ============================================================
struct DialogueTree {
    std::string treeId;                                          // NPC ID or dialogue ID
    std::unordered_map<std::string, DialogueNode> nodes;        // All nodes in tree
    std::string rootNodeId;                                      // Starting node ID
    std::string npcName;                                         // NPC name for display

    DialogueTree() = default;
};

// ============================================================
// DialogueSystem: Manages all NPC conversations
// ============================================================
class DialogueSystem {
public:
    DialogueSystem();
    ~DialogueSystem();

    // Dialogue Tree Management
    void registerDialogueTree(const DialogueTree& tree);
    const DialogueTree* getDialogueTree(uint32_t npcId) const;
    const DialogueNode* getDialogueNode(uint32_t npcId, const std::string& nodeId) const;

    // Conversation Control
    bool startConversation(std::shared_ptr<NPC> npc);
    bool selectOption(uint32_t npcId, int optionIndex);
    bool endConversation(uint32_t npcId);

    // Conversation State
    bool isConversationActive(uint32_t npcId) const;
    const DialogueNode* getCurrentNode(uint32_t npcId) const;
    std::string getCurrentNodeSpeech(uint32_t npcId) const;
    std::vector<std::string> getCurrentOptions(uint32_t npcId) const;

    // Dialogue Initialization
    void initializeDialogues();
    void clearDialogues() { dialogueTrees.clear(); }

private:
    std::unordered_map<uint32_t, DialogueTree> dialogueTrees;

    // Conversation state tracking
    struct ConversationState {
        uint32_t npcId = 0;
        std::string currentNodeId;
        bool isActive = false;
        float conversationTime = 0.0f;
    };

    std::unordered_map<uint32_t, ConversationState> activeConversations;

    ConversationState* findOrCreateConversation(uint32_t npcId);

    // Dialogue tree builders (Phase 4)
    void initializeCompanionDialogue();
    void initializeMerchantDialogue();
    void initializeGuardDialogue();
    void initializeMageDialogue();
};
