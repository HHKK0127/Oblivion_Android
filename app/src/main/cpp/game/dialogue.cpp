#include "dialogue.h"
#include <android/log.h>

#define LOG_TAG "DialogueManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

DialogueManager::DialogueManager()
    : currentDialogue(nullptr), m_factionRankProvider(nullptr) {
    LOGD("DialogueManager created");
}

DialogueManager::~DialogueManager() {
    clearDialogues();
    LOGD("DialogueManager destroyed");
}

std::shared_ptr<Dialogue> DialogueManager::createDialogue(uint32_t npcId,
                                                         const std::string& npcName,
                                                         const std::string& greeting) {
    if (dialogues.find(npcId) != dialogues.end()) {
        LOGE("Dialogue already exists for NPC: %u", npcId);
        return nullptr;
    }

    auto dialogue = std::make_shared<Dialogue>();
    dialogue->npcId = npcId;
    dialogue->npcName = npcName;
    dialogue->greeting = greeting;

    dialogues[npcId] = dialogue;
    LOGD("Dialogue created for NPC: %s (ID=%u)", npcName.c_str(), npcId);

    return dialogue;
}

std::shared_ptr<Dialogue> DialogueManager::getDialogue(uint32_t npcId) {
    auto it = dialogues.find(npcId);
    if (it != dialogues.end()) {
        return it->second;
    }
    return nullptr;
}

bool DialogueManager::startDialogue(uint32_t npcId) {
    auto dialogue = getDialogue(npcId);
    if (!dialogue) {
        LOGE("No dialogue found for NPC: %u", npcId);
        return false;
    }

    if (currentDialogue != nullptr) {
        endCurrentDialogue();
    }

    currentDialogue = dialogue;
    dialogue->isActive = true;
    dialogue->selectedTopicIndex = -1;

    LOGD("Dialogue started with %s: \"%s\"", dialogue->npcName.c_str(), dialogue->greeting.c_str());
    return true;
}

void DialogueManager::endCurrentDialogue() {
    if (currentDialogue) {
        LOGD("Dialogue ended with %s", currentDialogue->npcName.c_str());
        currentDialogue->end();
        currentDialogue = nullptr;
    }
}

void DialogueManager::selectTopic(int topicIndex) {
    if (!currentDialogue) {
        LOGD("No active dialogue to select topic from");
        return;
    }

    if (topicIndex >= 0 && topicIndex < (int)currentDialogue->topics.size()) {
        currentDialogue->selectTopic(topicIndex);
        LOGD("Selected topic %d: %s", topicIndex,
             currentDialogue->topics[topicIndex].topicText.c_str());
    } else {
        LOGE("Invalid topic index: %d", topicIndex);
    }
}

std::string DialogueManager::getCurrentResponse() const {
    if (!currentDialogue) {
        return "";
    }

    auto topic = currentDialogue->getSelectedTopic();
    if (topic) {
        return topic->responseText;
    }
    return "";
}

void DialogueManager::clearDialogues() {
    endCurrentDialogue();
    dialogues.clear();
    LOGD("All dialogues cleared");
}

void DialogueManager::loadDialoguesFromESM(const oblivion::ESMManager& esmMgr,
                                          std::function<std::vector<uint32_t>(uint32_t)> npcFactionLookup) {
    const auto& esmDialogs = esmMgr.getAllDialogs();
    size_t loaded = 0;
    size_t topicCount = 0;
    size_t factionGated = 0;

    for (const auto& dia : esmDialogs) {
        if (dia.formID == 0) continue;

        // Use DIAL formID as synthetic NPC ID for dialogue lookup
        uint32_t npcId = dia.formID;

        auto dialogue = createDialogue(npcId, dia.fullName, dia.fullName);
        dialogue->esmDialFormID = dia.formID;

        // Attach NPC faction memberships (for branching)
        if (npcFactionLookup) {
            dialogue->npcFactionFormIDs = npcFactionLookup(dia.formID);
        }

        // Convert INFO records to DialogueTopics with faction branching
        for (const auto& info : dia.infos) {
            std::string topicText = !info.promptText.empty()
                                    ? info.promptText
                                    : info.editorID;
            std::string responseText = info.responseText;

            DialogueTopic topic(info.editorID, topicText, responseText, info.questFormID != 0);
            topic.factionFormID = info.factionFormID;
            topic.factionRank = info.factionRank;

            if (info.factionFormID != 0) {
                ++factionGated;
            }

            dialogue->addTopic(topic);
            ++topicCount;
        }

        ++loaded;
    }

    LOGI("DialogueManager: Loaded %zu dialogues, %zu topics, %zu faction-gated from ESM",
         loaded, topicCount, factionGated);
}

std::vector<int> DialogueManager::getAvailableTopics() const {
    std::vector<int> result;
    if (!currentDialogue) return result;

    for (int i = 0; i < static_cast<int>(currentDialogue->topics.size()); ++i) {
        if (!m_factionRankProvider) {
            result.push_back(i);
            continue;
        }

        if (currentDialogue->isTopicAvailable(i, m_factionRankProvider)) {
            result.push_back(i);
        }
    }

    LOGD("Dialogue: %zu / %zu topics available after faction filtering",
         result.size(), currentDialogue->topics.size());
    return result;
}

void DialogueManager::logDialogueStats() const {
    LOGD("===== Dialogue Statistics =====");
    LOGD("Total NPCs with dialogue: %zu", dialogues.size());

    if (currentDialogue) {
        LOGD("Current Dialogue: %s", currentDialogue->npcName.c_str());
        LOGD("  Greeting: \"%s\"", currentDialogue->greeting.c_str());
        LOGD("  Topics: %zu", currentDialogue->topics.size());

        for (size_t i = 0; i < currentDialogue->topics.size(); ++i) {
            const auto& topic = currentDialogue->topics[i];
            LOGD("    %zu. %s %s", i, topic.topicText.c_str(),
                 topic.isQuest ? "[QUEST]" : "");
        }
    } else {
        LOGD("No active dialogue");
    }

    LOGD("================================");
}
