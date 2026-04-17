#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

/**
 * Dialogue Topic - A single conversation topic
 */
struct DialogueTopic {
    std::string topicId;           // Unique topic ID
    std::string topicText;         // Display text for topic
    std::string responseText;      // NPC response
    bool isQuest;                  // Is this a quest-related topic?

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

private:
    std::unordered_map<uint32_t, std::shared_ptr<Dialogue>> dialogues;  // NPC ID -> Dialogue
    std::shared_ptr<Dialogue> currentDialogue;  // Currently active dialogue

    void logDialogueStats() const;
};
