#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <android/log.h>

#define DH_LOG_TAG "DialogueHistory"
#ifdef ENABLE_DEBUG_LOGS
#define DH_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DH_LOG_TAG, __VA_ARGS__)
#else
#define DH_LOGD(...) do {} while(0)
#endif
#define DH_LOGI(...) __android_log_print(ANDROID_LOG_INFO, DH_LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace dialogue {

// ============================================================================
// Single dialogue history entry
// ============================================================================
struct DialogueHistoryEntry {
    uint32_t npcFormID = 0;
    std::string promptText;       // What the player said/selected
    std::string responseText;     // What the NPC responded
    uint32_t timestamp = 0;       // Game time when this occurred
    uint32_t topicFormID = 0;     // Topic that was discussed
};

// ============================================================================
// Per-NPC conversation summary
// ============================================================================
struct NPCConversationSummary {
    uint32_t npcFormID = 0;
    uint32_t totalInteractions = 0;
    uint32_t topicsDiscussed = 0;
    uint32_t lastInteractionTime = 0;
    std::vector<uint32_t> discussedTopicFormIDs;
};

// ============================================================================
// DialogueHistory - Memory-efficient circular buffer for conversation history
// ============================================================================
class DialogueHistory {
public:
    // Default capacity per NPC
    static constexpr size_t DEFAULT_ENTRIES_PER_NPC = 32;
    // Global history capacity
    static constexpr size_t DEFAULT_GLOBAL_CAPACITY = 256;

    DialogueHistory(size_t globalCapacity = DEFAULT_GLOBAL_CAPACITY,
                     size_t entriesPerNPC = DEFAULT_ENTRIES_PER_NPC);
    ~DialogueHistory();

    // Add a dialogue entry
    void addEntry(uint32_t npcFormID, const std::string& prompt,
                  const std::string& response, uint32_t topicFormID = 0);

    // Get recent entries for an NPC
    std::vector<DialogueHistoryEntry> getRecentEntries(uint32_t npcFormID,
                                                         size_t count = 10) const;

    // Get all entries for an NPC
    std::vector<DialogueHistoryEntry> getAllEntries(uint32_t npcFormID) const;

    // Get global recent entries (all NPCs)
    std::vector<DialogueHistoryEntry> getGlobalRecentEntries(size_t count = 20) const;

    // Check if player has talked to NPC before
    bool hasTalkedToNPC(uint32_t npcFormID) const;

    // Check if a specific topic has been discussed with NPC
    bool hasDiscussedTopic(uint32_t npcFormID, uint32_t topicFormID) const;

    // Get NPC conversation summary
    NPCConversationSummary getNPCSummary(uint32_t npcFormID) const;

    // Get all NPC summaries
    std::vector<NPCConversationSummary> getAllNPCSummaries() const;

    // Get total entry count
        size_t getTotalEntryCount() const {
            return globalWrapped ? globalCapacity : globalWriteIndex;
        }

    // Get NPC entry count
    size_t getNPCEntryCount(uint32_t npcFormID) const;

    // Clear history for an NPC
    void clearNPCHistory(uint32_t npcFormID);

    // Clear all history
    void clearAll();

    // Get current game time (for timestamps)
    void setGameTime(uint32_t time) { gameTime = time; }
    uint32_t getGameTime() const { return gameTime; }

    // Statistics
    size_t getNPCCount() const { return npcBuffers.size(); }

    // Logging
    void logStats() const;

private:
    size_t globalCapacity;
    size_t entriesPerNPC;
    uint32_t gameTime = 0;

    // Global circular buffer
    std::vector<DialogueHistoryEntry> globalBuffer;
    size_t globalWriteIndex = 0;
    bool globalWrapped = false;

    // Per-NPC circular buffers
    struct NPCBuffer {
        std::vector<DialogueHistoryEntry> entries;
        size_t writeIndex = 0;
        bool wrapped = false;
        uint32_t totalInteractions = 0;
    };
    std::unordered_map<uint32_t, NPCBuffer> npcBuffers;

    // Helper: add to global buffer
    void addToGlobalBuffer(const DialogueHistoryEntry& entry);

    // Helper: add to NPC buffer
    void addToNPCBuffer(uint32_t npcFormID, const DialogueHistoryEntry& entry);

    // Helper: get entries from circular buffer
    std::vector<DialogueHistoryEntry> getEntriesFromBuffer(
        const std::vector<DialogueHistoryEntry>& buffer,
        size_t writeIndex, bool wrapped, size_t count) const;
};

} // namespace dialogue
} // namespace oblivion
