#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../assets/esm_reader.h"

/**
 * Dialogue Topic - A single conversation topic
 */
struct DialogueTopic {
    std::string topicId;           // Unique topic ID
    std::string topicText;         // Display text for topic
    std::string responseText;      // NPC response
    bool isQuest;                  // Is this a quest-related topic?

    // Faction branching (ESM integration)
    uint32_t factionFormID = 0;    // Required faction to see this topic (0 = no requirement)
    int32_t factionRank = -1;      // Required faction rank (-1 = no requirement)

    DialogueTopic(const std::string& id, const std::string& text,
                  const std::string& response, bool quest = false)
        : topicId(id), topicText(text), responseText(response), isQuest(quest) {}
};

/**
 * Dialogue - Complete conversation with an NPC
 */
struct Dialogue {
    uint32_t npcId;                // NPC this dialogue belongs to
    std::string npcName;           // NPC name
    std::string greeting;          // Initial greeting
    std::vector<DialogueTopic> topics;  // Available topics
    bool isActive;                 // Is dialogue currently active?
    int selectedTopicIndex;        // Currently selected topic (-1 = none)

    // ESM data linkage
    uint32_t esmDialFormID = 0;    // Originating DIAL formID from ESM
    std::vector<uint32_t> npcFactionFormIDs;  // NPC's faction memberships (for branching)

    Dialogue()
        : npcId(0), isActive(false), selectedTopicIndex(-1) {}

    // Add dialogue topic
    void addTopic(const DialogueTopic& topic) {
        topics.push_back(topic);
    }

    // Get current selected topic
    DialogueTopic* getSelectedTopic() {
        if (selectedTopicIndex >= 0 && selectedTopicIndex < (int)topics.size()) {
            return &topics[selectedTopicIndex];
        }
        return nullptr;
    }

    // Select topic by index
    void selectTopic(int index) {
        if (index >= 0 && index < (int)topics.size()) {
            selectedTopicIndex = index;
        }
    }

    // End dialogue
    void end() {
        isActive = false;
        selectedTopicIndex = -1;
    }

    // Check if topic is available given NPC's faction memberships and ranks
    bool isTopicAvailable(int topicIndex,
                          const std::function<bool(uint32_t, int32_t)>& hasFactionRank) const {
        if (topicIndex < 0 || topicIndex >= (int)topics.size()) return false;
        const auto& topic = topics[topicIndex];
        if (topic.factionFormID == 0) return true;  // No faction requirement
        return hasFactionRank(topic.factionFormID, topic.factionRank);
    }
};

/**
 * Dialogue Manager - Manages all dialogues in the game
 */
class DialogueManager {
public:
    DialogueManager();
    ~DialogueManager();

    // Create dialogue for NPC
    std::shared_ptr<Dialogue> createDialogue(uint32_t npcId, const std::string& npcName,
                                            const std::string& greeting);

    // Get dialogue
    std::shared_ptr<Dialogue> getDialogue(uint32_t npcId);

    // Start dialogue
    bool startDialogue(uint32_t npcId);

    // End current dialogue
    void endCurrentDialogue();

    // Get current active dialogue
    std::shared_ptr<Dialogue> getCurrentDialogue() const { return currentDialogue; }

    // Select dialogue topic
    void selectTopic(int topicIndex);

    // Get response for current topic
    std::string getCurrentResponse() const;

    // Statistics
    size_t getDialogueCount() const { return dialogues.size(); }

    // Cleanup
    void clearDialogues();

    // ESM-driven dialogue loading
    // Loads dialogue trees from ESM DIAL/INFO records and attaches to NPCs
    // npcFactionLookup: function returning faction memberships for an NPC FormID
    void loadDialoguesFromESM(const oblivion::ESMManager& esmMgr,
                              std::function<std::vector<uint32_t>(uint32_t)> npcFactionLookup = nullptr);

    // Faction-based topic filtering for current dialogue
    // Returns indices of topics available given NPC's factions
    std::vector<int> getAvailableTopics() const;

    // Set/get faction membership provider (for branching)
    using FactionRankProvider = std::function<bool(uint32_t, int32_t)>;
    void setFactionRankProvider(FactionRankProvider provider) { m_factionRankProvider = provider; }

private:
    std::unordered_map<uint32_t, std::shared_ptr<Dialogue>> dialogues;  // NPC ID -> Dialogue
    std::shared_ptr<Dialogue> currentDialogue;  // Currently active dialogue
    FactionRankProvider m_factionRankProvider;  // Returns true if NPC has faction rank

    void logDialogueStats() const;
};
