#include "dialogue_tree.h"
#include <algorithm>
#include <cstring>

namespace oblivion {
namespace dialogue {

// ============================================================================
// TopicGroup
// ============================================================================

uint32_t TopicGroup::getBestNodeID() const {
    // Return the first node (highest priority after sorting)
    if (!nodeIDs.empty()) {
        return nodeIDs[0];
    }
    return 0;
}

// ============================================================================
// DialogueTree
// ============================================================================

DialogueTree::DialogueTree() = default;
DialogueTree::~DialogueTree() = default;

void DialogueTree::buildFromDialRecords(const std::vector<DialogueDialRecord>& dialRecords,
                                          uint32_t npcFormID) {
    this->npcFormID = npcFormID;
    nodes.clear();
    topicGroups.clear();
    categoryIndex.clear();
    greetingNodeID = 0;

    for (const auto& dial : dialRecords) {
        TopicCategory category = categorizeDialType(dial.type, dial.editorID);

        TopicGroup group;
        group.topicFormID = dial.formID;
        group.topicName = !dial.fullName.empty() ? dial.fullName : dial.editorID;
        group.category = category;
        group.dialogueType = dial.type;

        for (const auto& info : dial.infos) {
            // Create node from INFO record
            DialogueNode node;
            node.nodeID = nextNodeID++;
            node.infoFormID = info.formID;
            node.editorID = info.editorID;
            node.promptText = info.getPromptText();
            node.responseText = info.getResponseText();
            node.category = category;
            node.priority = info.priority;
            node.conditions = info.conditions;
            node.scriptConditions = info.scriptConditions;
            node.questFormID = info.questFormID;
            node.questStage = info.questStage;
            node.factionFormID = info.factionFormID;
            node.factionRank = info.factionRank;
            node.filterNPCFormID = info.filterNPCFormID;
            node.filterGender = info.filterGender;
            node.filterRaceFormID = info.filterRaceFormID;
            node.filterClassFormID = info.filterClassFormID;
            node.topicLinks = info.topicLinks;

            // Store node
            uint32_t nodeID = node.nodeID;
            nodes[nodeID] = std::move(node);
            group.nodeIDs.push_back(nodeID);

            // Track greeting node
            if (category == TopicCategory::Greeting && greetingNodeID == 0) {
                greetingNodeID = nodeID;
            }

            // Update category index
            categoryIndex[category].push_back(nodeID);
        }

        // Sort nodes in group by priority (descending)
        std::sort(group.nodeIDs.begin(), group.nodeIDs.end(),
            [this](uint32_t a, uint32_t b) {
                return nodes[a].priority > nodes[b].priority;
            });

        topicGroups[dial.formID] = std::move(group);
    }

    // Build parent-child links
    buildNodeLinks();

    DT_LOGI("DialogueTree built: NPC=0x%08X nodes=%zu topics=%zu",
            npcFormID, nodes.size(), topicGroups.size());
}

uint32_t DialogueTree::addNode(const DialogueNode& node) {
    uint32_t id = nextNodeID++;
    DialogueNode copy = node;
    copy.nodeID = id;
    nodes[id] = std::move(copy);
    return id;
}

DialogueNode* DialogueTree::getNode(uint32_t nodeID) {
    auto it = nodes.find(nodeID);
    return (it != nodes.end()) ? &it->second : nullptr;
}

const DialogueNode* DialogueTree::getNode(uint32_t nodeID) const {
    auto it = nodes.find(nodeID);
    return (it != nodes.end()) ? &it->second : nullptr;
}

void DialogueTree::addTopicGroup(const TopicGroup& group) {
    topicGroups[group.topicFormID] = group;
}

const TopicGroup* DialogueTree::getTopicGroup(uint32_t topicFormID) const {
    auto it = topicGroups.find(topicFormID);
    return (it != topicGroups.end()) ? &it->second : nullptr;
}

std::vector<const TopicGroup*> DialogueTree::getTopicGroupsByCategory(TopicCategory category) const {
    std::vector<const TopicGroup*> result;
    for (const auto& [fid, group] : topicGroups) {
        if (group.category == category) {
            result.push_back(&group);
        }
    }
    return result;
}

std::vector<const TopicGroup*> DialogueTree::getAllTopicGroups() const {
    std::vector<const TopicGroup*> result;
    result.reserve(topicGroups.size());
    for (const auto& [fid, group] : topicGroups) {
        result.push_back(&group);
    }
    return result;
}

const DialogueNode* DialogueTree::getGreetingNode() const {
    if (greetingNodeID != 0) {
        return getNode(greetingNodeID);
    }
    return nullptr;
}

std::vector<const DialogueNode*> DialogueTree::getAvailableNodes() const {
    std::vector<const DialogueNode*> result;
    for (const auto& [id, node] : nodes) {
        if (node.isAvailable()) {
            result.push_back(&node);
        }
    }
    // Sort by priority descending
    std::sort(result.begin(), result.end(),
        [](const DialogueNode* a, const DialogueNode* b) {
            return a->priority > b->priority;
        });
    return result;
}

std::vector<const DialogueNode*> DialogueTree::getNodesForTopic(uint32_t topicFormID) const {
    std::vector<const DialogueNode*> result;
    auto it = topicGroups.find(topicFormID);
    if (it == topicGroups.end()) return result;

    for (uint32_t nodeID : it->second.nodeIDs) {
        const DialogueNode* node = getNode(nodeID);
        if (node) {
            result.push_back(node);
        }
    }
    return result;
}

void DialogueTree::markNodeSpoken(uint32_t nodeID) {
    auto it = nodes.find(nodeID);
    if (it != nodes.end()) {
        it->second.hasBeenSpoken = true;
        it->second.timesSpoken++;
    }
}

void DialogueTree::resetSpokenStates() {
    for (auto& [id, node] : nodes) {
        node.hasBeenSpoken = false;
        node.timesSpoken = 0;
    }
}

size_t DialogueTree::getSpokenCount() const {
    size_t count = 0;
    for (const auto& [id, node] : nodes) {
        if (node.hasBeenSpoken) ++count;
    }
    return count;
}

void DialogueTree::logTreeStats() const {
    DT_LOGI("===== DialogueTree Stats =====");
    DT_LOGI("NPC FormID: 0x%08X", npcFormID);
    DT_LOGI("Total nodes: %zu", nodes.size());
    DT_LOGI("Topic groups: %zu", topicGroups.size());
    DT_LOGI("Spoken nodes: %zu", getSpokenCount());
    DT_LOGI("Greeting node ID: %u", greetingNodeID);

    for (const auto& [fid, group] : topicGroups) {
        DT_LOGI("  Topic 0x%08X: %s (%zu nodes, category=%d)",
                fid, group.topicName.c_str(), group.nodeIDs.size(),
                static_cast<int>(group.category));
    }
    DT_LOGI("==============================");
}

TopicCategory DialogueTree::categorizeDialType(DialogueType type,
                                                  const std::string& editorID) const {
    switch (type) {
        case DialogueType::Topic:
            // Check editor ID for greeting/farewell hints
            if (editorID.find("Greet") != std::string::npos ||
                editorID.find("greet") != std::string::npos) {
                return TopicCategory::Greeting;
            }
            if (editorID.find("Bye") != std::string::npos ||
                editorID.find("bye") != std::string::npos ||
                editorID.find("Farewell") != std::string::npos) {
                return TopicCategory::Farewell;
            }
            return TopicCategory::Specific;

        case DialogueType::Conversation:
            return TopicCategory::Flavor;

        case DialogueType::Combat:
            return TopicCategory::Combat;

        case DialogueType::Persuasion:
            return TopicCategory::Persuasion;

        case DialogueType::Detection:
            return TopicCategory::Detection;

        case DialogueType::Service:
            return TopicCategory::Service;

        case DialogueType::Misc:
        default:
            return TopicCategory::Flavor;
    }
}

void DialogueTree::buildNodeLinks() {
    // Build parent-child relationships from topic links
    for (auto& [id, node] : nodes) {
        for (const auto& link : node.topicLinks) {
            // Find nodes belonging to the linked topic
            auto it = topicGroups.find(link.topicFormID);
            if (it != topicGroups.end()) {
                for (uint32_t childID : it->second.nodeIDs) {
                    node.childNodeIDs.push_back(childID);
                    auto childIt = nodes.find(childID);
                    if (childIt != nodes.end()) {
                        childIt->second.parentNodeID = id;
                    }
                }
            }
        }
    }
}

} // namespace dialogue
} // namespace oblivion
