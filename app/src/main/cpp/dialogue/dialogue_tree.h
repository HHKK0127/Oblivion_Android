#pragma once

#include "dialogue_record.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <android/log.h>

#define DT_LOG_TAG "DialogueTree"
#ifdef ENABLE_DEBUG_LOGS
#define DT_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DT_LOG_TAG, __VA_ARGS__)
#else
#define DT_LOGD(...) do {} while(0)
#endif
#define DT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, DT_LOG_TAG, __VA_ARGS__)
#define DT_LOGW(...) __android_log_print(ANDROID_LOG_WARN, DT_LOG_TAG, __VA_ARGS__)
#define DT_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DT_LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace dialogue {

// ============================================================================
// Topic category
// ============================================================================
enum class TopicCategory : uint8_t {
    Greeting,       // Initial NPC greeting
    Farewell,       // Goodbye
    Flavor,         // Ambient/flavor dialogue
    Specific,       // Specific conversation topic
    Quest,          // Quest-related dialogue
    Service,        // Merchant/trainer service
    Persuasion,     // Persuasion options
    Combat,         // Combat taunts
    Detection       // Detection dialogue
};

// ============================================================================
// Dialogue Node - A single node in the dialogue tree
// ============================================================================
struct DialogueNode {
    uint32_t nodeID = 0;                // Unique node ID
    uint32_t infoFormID = 0;            // Source INFO record FormID
    std::string editorID;               // Editor ID
    std::string promptText;             // Player prompt text
    std::string responseText;           // NPC response text
    TopicCategory category = TopicCategory::Specific;
    int32_t priority = 0;               // Higher = checked first

    // Conditions
    std::vector<DialogueCondition> conditions;
    std::vector<ScriptVariableCondition> scriptConditions;

    // Links to other nodes
    std::vector<uint32_t> childNodeIDs;  // Nodes accessible from this one
    uint32_t parentNodeID = 0;           // Parent node (0 = root)

    // Quest linkage
    uint32_t questFormID = 0;
    int32_t questStage = -1;

    // Faction requirement
    uint32_t factionFormID = 0;
    int32_t factionRank = -1;

    // Filter data
    uint32_t filterNPCFormID = 0;
    uint8_t filterGender = 0xFF;
    uint32_t filterRaceFormID = 0;
    uint32_t filterClassFormID = 0;

    // State
    bool hasBeenSpoken = false;           // Player has seen this node
    uint32_t timesSpoken = 0;            // How many times spoken

    // Script to execute on selection
    uint32_t resultScriptFormID = 0;

    // Topic links (TCLT)
    std::vector<TopicLink> topicLinks;

    // Check if this node is available for a given NPC context
    bool isAvailable() const { return !hasBeenSpoken || category == TopicCategory::Greeting; }
};

// ============================================================================
// Topic Group - Groups nodes by topic type
// ============================================================================
struct TopicGroup {
    uint32_t topicFormID = 0;           // DIAL FormID
    std::string topicName;              // Topic display name
    TopicCategory category = TopicCategory::Specific;
    DialogueType dialogueType = DialogueType::Topic;
    std::vector<uint32_t> nodeIDs;      // Nodes in this topic

    // Get the highest priority available node
    uint32_t getBestNodeID() const;
};

// ============================================================================
// DialogueTree - Manages the complete dialogue tree for an NPC
// ============================================================================
class DialogueTree {
public:
    DialogueTree();
    ~DialogueTree();

    // Build tree from parsed DIAL records
    void buildFromDialRecords(const std::vector<DialogueDialRecord>& dialRecords,
                               uint32_t npcFormID);

    // Add a single node
    uint32_t addNode(const DialogueNode& node);

    // Get node by ID
    DialogueNode* getNode(uint32_t nodeID);
    const DialogueNode* getNode(uint32_t nodeID) const;

    // Get all nodes
    const std::unordered_map<uint32_t, DialogueNode>& getAllNodes() const { return nodes; }

    // Topic management
    void addTopicGroup(const TopicGroup& group);
    const TopicGroup* getTopicGroup(uint32_t topicFormID) const;
    std::vector<const TopicGroup*> getTopicGroupsByCategory(TopicCategory category) const;
    std::vector<const TopicGroup*> getAllTopicGroups() const;

    // Get greeting node
    const DialogueNode* getGreetingNode() const;

    // Get available nodes for current context
    std::vector<const DialogueNode*> getAvailableNodes() const;

    // Get nodes for a specific topic
    std::vector<const DialogueNode*> getNodesForTopic(uint32_t topicFormID) const;

    // Mark node as spoken
    void markNodeSpoken(uint32_t nodeID);

    // Reset all spoken states
    void resetSpokenStates();

    // NPC association
    uint32_t getNPCFormID() const { return npcFormID; }
    void setNPCFormID(uint32_t id) { npcFormID = id; }

    // Statistics
    size_t getNodeCount() const { return nodes.size(); }
    size_t getTopicCount() const { return topicGroups.size(); }
    size_t getSpokenCount() const;

    // Logging
    void logTreeStats() const;

private:
    uint32_t npcFormID = 0;
    uint32_t nextNodeID = 1;

    // Node storage
    std::unordered_map<uint32_t, DialogueNode> nodes;

    // Topic groups
    std::unordered_map<uint32_t, TopicGroup> topicGroups;

    // Category index
    std::unordered_map<TopicCategory, std::vector<uint32_t>> categoryIndex;

    // Greeting node ID
    uint32_t greetingNodeID = 0;

    // Helper: categorize a DIAL type
    TopicCategory categorizeDialType(DialogueType type, const std::string& editorID) const;

    // Helper: build parent-child links
    void buildNodeLinks();
};

} // namespace dialogue
} // namespace oblivion
