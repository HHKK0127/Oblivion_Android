#include "dialogue_history.h"
#include <algorithm>

namespace oblivion {
namespace dialogue {

// ============================================================================
// DialogueHistory
// ============================================================================

DialogueHistory::DialogueHistory(size_t globalCapacity, size_t entriesPerNPC)
    : globalCapacity(globalCapacity), entriesPerNPC(entriesPerNPC) {
    globalBuffer.resize(globalCapacity);
    DH_LOGI("DialogueHistory created: globalCap=%zu, perNPC=%zu", globalCapacity, entriesPerNPC);
}

DialogueHistory::~DialogueHistory() = default;

void DialogueHistory::addEntry(uint32_t npcFormID, const std::string& prompt,
                                 const std::string& response, uint32_t topicFormID) {
    DialogueHistoryEntry entry;
    entry.npcFormID = npcFormID;
    entry.promptText = prompt;
    entry.responseText = response;
    entry.timestamp = gameTime;
    entry.topicFormID = topicFormID;

    addToGlobalBuffer(entry);
    addToNPCBuffer(npcFormID, entry);

    DH_LOGD("Added history entry: NPC=0x%08X topic=0x%08X", npcFormID, topicFormID);
}

std::vector<DialogueHistoryEntry> DialogueHistory::getRecentEntries(
    uint32_t npcFormID, size_t count) const {
    auto it = npcBuffers.find(npcFormID);
    if (it == npcBuffers.end()) {
        return {};
    }
    const auto& buf = it->second;
    return getEntriesFromBuffer(buf.entries, buf.writeIndex, buf.wrapped, count);
}

std::vector<DialogueHistoryEntry> DialogueHistory::getAllEntries(uint32_t npcFormID) const {
    auto it = npcBuffers.find(npcFormID);
    if (it == npcBuffers.end()) {
        return {};
    }

    const auto& buf = it->second;
    std::vector<DialogueHistoryEntry> result;

    if (buf.wrapped) {
        // Buffer has wrapped - entries are from writeIndex to end, then 0 to writeIndex
        result.reserve(buf.entries.size());
        for (size_t i = buf.writeIndex; i < buf.entries.size(); ++i) {
            if (!buf.entries[i].responseText.empty()) {
                result.push_back(buf.entries[i]);
            }
        }
        for (size_t i = 0; i < buf.writeIndex; ++i) {
            if (!buf.entries[i].responseText.empty()) {
                result.push_back(buf.entries[i]);
            }
        }
    } else {
        result.reserve(buf.writeIndex);
        for (size_t i = 0; i < buf.writeIndex; ++i) {
            if (!buf.entries[i].responseText.empty()) {
                result.push_back(buf.entries[i]);
            }
        }
    }

    return result;
}

std::vector<DialogueHistoryEntry> DialogueHistory::getGlobalRecentEntries(size_t count) const {
    return getEntriesFromBuffer(globalBuffer, globalWriteIndex, globalWrapped, count);
}

bool DialogueHistory::hasTalkedToNPC(uint32_t npcFormID) const {
    auto it = npcBuffers.find(npcFormID);
    return it != npcBuffers.end() && (it->second.wrapped || it->second.writeIndex > 0);
}

bool DialogueHistory::hasDiscussedTopic(uint32_t npcFormID, uint32_t topicFormID) const {
    auto it = npcBuffers.find(npcFormID);
    if (it == npcBuffers.end()) return false;

    const auto& buf = it->second;
    size_t count = buf.wrapped ? buf.entries.size() : buf.writeIndex;

    for (size_t i = 0; i < count; ++i) {
        if (buf.entries[i].topicFormID == topicFormID) {
            return true;
        }
    }
    return false;
}

NPCConversationSummary DialogueHistory::getNPCSummary(uint32_t npcFormID) const {
    NPCConversationSummary summary;
    summary.npcFormID = npcFormID;

    auto it = npcBuffers.find(npcFormID);
    if (it == npcBuffers.end()) return summary;

    const auto& buf = it->second;
    summary.totalInteractions = buf.totalInteractions;
    summary.lastInteractionTime = gameTime;

    // Count unique topics
    std::unordered_map<uint32_t, bool> seenTopics;
    size_t count = buf.wrapped ? buf.entries.size() : buf.writeIndex;
    for (size_t i = 0; i < count; ++i) {
        if (buf.entries[i].topicFormID != 0) {
            if (!seenTopics[buf.entries[i].topicFormID]) {
                seenTopics[buf.entries[i].topicFormID] = true;
                summary.discussedTopicFormIDs.push_back(buf.entries[i].topicFormID);
            }
        }
    }
    summary.topicsDiscussed = summary.discussedTopicFormIDs.size();

    return summary;
}

std::vector<NPCConversationSummary> DialogueHistory::getAllNPCSummaries() const {
    std::vector<NPCConversationSummary> result;
    result.reserve(npcBuffers.size());
    for (const auto& [npcFormID, buf] : npcBuffers) {
        result.push_back(getNPCSummary(npcFormID));
    }
    return result;
}

size_t DialogueHistory::getNPCEntryCount(uint32_t npcFormID) const {
    auto it = npcBuffers.find(npcFormID);
    if (it == npcBuffers.end()) return 0;
    const auto& buf = it->second;
    return buf.wrapped ? buf.entries.size() : buf.writeIndex;
}

void DialogueHistory::clearNPCHistory(uint32_t npcFormID) {
    npcBuffers.erase(npcFormID);
    DH_LOGD("Cleared history for NPC 0x%08X", npcFormID);
}

void DialogueHistory::clearAll() {
    globalBuffer.clear();
    globalBuffer.resize(globalCapacity);
    globalWriteIndex = 0;
    globalWrapped = false;
    npcBuffers.clear();
    DH_LOGI("All dialogue history cleared");
}

void DialogueHistory::logStats() const {
    DH_LOGI("===== DialogueHistory Stats =====");
    DH_LOGI("Global entries: %zu/%zu (wrapped=%d)",
            globalWrapped ? globalCapacity : globalWriteIndex,
            globalCapacity, globalWrapped);
    DH_LOGI("NPCs tracked: %zu", npcBuffers.size());

    for (const auto& [npcFormID, buf] : npcBuffers) {
        size_t count = buf.wrapped ? buf.entries.size() : buf.writeIndex;
        DH_LOGI("  NPC 0x%08X: %zu entries, %u interactions",
                npcFormID, count, buf.totalInteractions);
    }
    DH_LOGI("=================================");
}

// ============================================================================
// Private helpers
// ============================================================================

void DialogueHistory::addToGlobalBuffer(const DialogueHistoryEntry& entry) {
    globalBuffer[globalWriteIndex] = entry;
    globalWriteIndex = (globalWriteIndex + 1) % globalCapacity;
    if (globalWriteIndex == 0) globalWrapped = true;
}

void DialogueHistory::addToNPCBuffer(uint32_t npcFormID, const DialogueHistoryEntry& entry) {
    auto& buf = npcBuffers[npcFormID];

    // Initialize buffer if needed
    if (buf.entries.empty()) {
        buf.entries.resize(entriesPerNPC);
        buf.writeIndex = 0;
        buf.wrapped = false;
    }

    buf.entries[buf.writeIndex] = entry;
    buf.writeIndex = (buf.writeIndex + 1) % entriesPerNPC;
    if (buf.writeIndex == 0) buf.wrapped = true;
    buf.totalInteractions++;
}

std::vector<DialogueHistoryEntry> DialogueHistory::getEntriesFromBuffer(
    const std::vector<DialogueHistoryEntry>& buffer,
    size_t writeIndex, bool wrapped, size_t count) const {

    std::vector<DialogueHistoryEntry> result;
    if (buffer.empty()) return result;

    size_t totalEntries = wrapped ? buffer.size() : writeIndex;
    size_t startIdx = 0;

    if (totalEntries > count) {
        startIdx = wrapped
            ? (writeIndex + buffer.size() - count) % buffer.size()
            : writeIndex - count;
    }

    result.reserve(std::min(count, totalEntries));

    for (size_t i = 0; i < std::min(count, totalEntries); ++i) {
        size_t idx = (startIdx + i) % buffer.size();
        if (!buffer[idx].responseText.empty()) {
            result.push_back(buffer[idx]);
        }
    }

    return result;
}

} // namespace dialogue
} // namespace oblivion
