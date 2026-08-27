#include "dialogue_runner.h"
#include <algorithm>
#include <cctype>

namespace oblivion {
namespace dialogue {

// ============================================================================
// DialogueRunner
// ============================================================================

DialogueRunner::DialogueRunner() = default;
DialogueRunner::~DialogueRunner() = default;

bool DialogueRunner::startDialogue(uint32_t npcFormID, DialogueTree& tree) {
    if (state != DialogueState::Idle) {
        endDialogue();
    }

    currentNPCFormID = npcFormID;
    currentTree = &tree;
    topicsDiscussed = 0;

    DR_LOGI("Starting dialogue with NPC 0x%08X (nodes=%zu)", npcFormID, tree.getNodeCount());

    // Select greeting
    if (filterEngine) {
        const DialogueNode* greeting = filterEngine->selectGreeting(tree);
        if (greeting) {
            currentResponse = greeting->responseText;
            currentNodeID = greeting->nodeID;
            tree.markNodeSpoken(greeting->nodeID);

            // Record in history
            if (history) {
                history->addEntry(npcFormID, greeting->promptText, greeting->responseText);
            }

            // Emit greeting event
            DialogueEvent event;
            event.type = DialogueEvent::Type::Greeting;
            event.text = currentResponse;
            event.speakerFormID = npcFormID;
            emitEvent(event);

            transitionToState(DialogueState::Greeting);
        } else {
            DR_LOGW("No greeting found for NPC 0x%08X", npcFormID);
            currentResponse = "...";
            transitionToState(DialogueState::Greeting);
        }
    }

    // Update available topics
    updateAvailableTopics();

    // Emit topic list
    {
        DialogueEvent event;
        event.type = DialogueEvent::Type::TopicList;
        event.options = availableTopicNames;
        emitEvent(event);
    }

    transitionToState(DialogueState::TopicSelection);
    return true;
}

void DialogueRunner::endDialogue() {
    if (state == DialogueState::Idle) return;

    DR_LOGI("Ending dialogue with NPC 0x%08X (topics discussed=%zu)",
            currentNPCFormID, topicsDiscussed);

    // Emit farewell event
    DialogueEvent event;
    event.type = DialogueEvent::Type::Farewell;
    event.text = "Goodbye.";
    event.speakerFormID = currentNPCFormID;
    emitEvent(event);

    // Execute farewell scripts if any
    if (currentTree) {
        auto farewellGroups = currentTree->getTopicGroupsByCategory(TopicCategory::Farewell);
        for (const auto* group : farewellGroups) {
            if (filterEngine) {
                const DialogueNode* farewell = filterEngine->selectBestResponseForTopic(
                    *currentTree, group->topicFormID);
                if (farewell && farewell->resultScriptFormID != 0) {
                    executeResultScript(farewell->resultScriptFormID);
                }
            }
        }
    }

    state = DialogueState::Idle;
    currentNPCFormID = 0;
    currentTree = nullptr;
    currentResponse.clear();
    currentNodeID = 0;
    availableTopicFormIDs.clear();
    availableTopicNames.clear();
    currentChoices.clear();
    scriptRunning = false;
}

bool DialogueRunner::selectTopic(int topicIndex) {
    if (state != DialogueState::TopicSelection && state != DialogueState::Greeting) {
        DR_LOGW("Cannot select topic in state %d", static_cast<int>(state));
        return false;
    }

    if (topicIndex < 0 || topicIndex >= static_cast<int>(availableTopicFormIDs.size())) {
        DR_LOGE("Invalid topic index: %d (available=%zu)", topicIndex, availableTopicFormIDs.size());
        return false;
    }

    return selectTopicByFormID(availableTopicFormIDs[topicIndex]);
}

bool DialogueRunner::selectTopicByFormID(uint32_t topicFormID) {
    if (!currentTree || !filterEngine) {
        DR_LOGE("No active dialogue or filter engine");
        return false;
    }

    DR_LOGD("Selecting topic 0x%08X", topicFormID);

    // Find best response for this topic
    const DialogueNode* response = filterEngine->selectBestResponseForTopic(
        *currentTree, topicFormID);

    if (!response) {
        DR_LOGW("No response found for topic 0x%08X", topicFormID);
        return false;
    }

    // Update state
    currentResponse = response->responseText;
    currentNodeID = response->nodeID;
    currentTree->markNodeSpoken(response->nodeID);
    topicsDiscussed++;

    // Record in history
    if (history) {
        history->addEntry(currentNPCFormID, response->promptText, response->responseText);
    }

    // Emit response event
    {
        DialogueEvent event;
        event.type = DialogueEvent::Type::Response;
        event.text = currentResponse;
        event.speakerFormID = currentNPCFormID;
        emitEvent(event);
    }

    // Check for choices (branching)
    if (!response->childNodeIDs.empty()) {
        currentChoices.clear();
        for (uint32_t childID : response->childNodeIDs) {
            const DialogueNode* child = currentTree->getNode(childID);
            if (child) {
                DialogueChoice choice;
                choice.text = !child->promptText.empty() ? child->promptText : child->editorID;
                choice.targetNodeID = childID;
                choice.topicFormID = 0;
                currentChoices.push_back(choice);
            }
        }

        if (!currentChoices.empty()) {
            DialogueEvent event;
            event.type = DialogueEvent::Type::ChoiceList;
            event.choices = currentChoices;
            for (const auto& c : currentChoices) {
                event.options.push_back(c.text);
            }
            emitEvent(event);
            transitionToState(DialogueState::Choice);
            return true;
        }
    }

    // Execute result script if any
    if (response->resultScriptFormID != 0) {
        executeResultScript(response->resultScriptFormID);
    }

    // Apply Speechcraft bonus for successful persuasion
    if (response->category == TopicCategory::Persuasion) {
        applySpeechcraftBonus(0.5f);
    }

    // Update available topics and return to selection
    updateAvailableTopics();

    {
        DialogueEvent event;
        event.type = DialogueEvent::Type::TopicList;
        event.options = availableTopicNames;
        emitEvent(event);
    }

    transitionToState(DialogueState::TopicSelection);
    return true;
}

bool DialogueRunner::selectChoice(int choiceIndex) {
    if (state != DialogueState::Choice) {
        DR_LOGW("Cannot select choice in state %d", static_cast<int>(state));
        return false;
    }

    if (choiceIndex < 0 || choiceIndex >= static_cast<int>(currentChoices.size())) {
        DR_LOGE("Invalid choice index: %d", choiceIndex);
        return false;
    }

    const auto& choice = currentChoices[choiceIndex];

    // Navigate to target node
    if (choice.targetNodeID != 0 && currentTree) {
        const DialogueNode* targetNode = currentTree->getNode(choice.targetNodeID);
        if (targetNode) {
            currentResponse = targetNode->responseText;
            currentNodeID = targetNode->nodeID;
            currentTree->markNodeSpoken(targetNode->nodeID);

            // Record in history
            if (history) {
                history->addEntry(currentNPCFormID, choice.text, targetNode->responseText);
            }

            // Emit response
            DialogueEvent event;
            event.type = DialogueEvent::Type::Response;
            event.text = currentResponse;
            event.speakerFormID = currentNPCFormID;
            emitEvent(event);

            // Execute result script
            if (targetNode->resultScriptFormID != 0) {
                executeResultScript(targetNode->resultScriptFormID);
            }
        }
    } else if (choice.topicFormID != 0) {
        // Navigate to a different topic
        return selectTopicByFormID(choice.topicFormID);
    }

    // Return to topic selection
    currentChoices.clear();
    updateAvailableTopics();

    {
        DialogueEvent event;
        event.type = DialogueEvent::Type::TopicList;
        event.options = availableTopicNames;
        emitEvent(event);
    }

    transitionToState(DialogueState::TopicSelection);
    return true;
}

bool DialogueRunner::processTextInput(const std::string& input) {
    if (state == DialogueState::Idle) {
        DR_LOGW("No active dialogue for text input");
        return false;
    }

    DR_LOGD("Processing text input: \"%s\"", input.c_str());

    // Try to match input to a topic
    if (matchTopicFromText(input)) {
        return true;
    }

    // If no match, show available topics
    DR_LOGD("No topic match for input, showing available topics");
    updateAvailableTopics();

    DialogueEvent event;
    event.type = DialogueEvent::Type::TopicList;
    event.options = availableTopicNames;
    emitEvent(event);

    return false;
}

void DialogueRunner::update(float deltaTime) {
    if (state == DialogueState::Idle) return;

    // Handle script execution timeout
    if (scriptRunning) {
        scriptTimer += deltaTime;
        if (scriptTimer >= SCRIPT_TIMEOUT) {
            DR_LOGW("Script execution timed out after %.1f seconds", scriptTimer);
            scriptRunning = false;
            scriptTimer = 0.0f;

            DialogueEvent event;
            event.type = DialogueEvent::Type::ScriptResult;
            event.text = "Script timed out";
            event.success = false;
            emitEvent(event);
        }
    }
}

std::vector<std::string> DialogueRunner::getAvailableTopicNames() const {
    return availableTopicNames;
}

std::vector<uint32_t> DialogueRunner::getAvailableTopicFormIDs() const {
    return availableTopicFormIDs;
}

// ============================================================================
// Private helpers
// ============================================================================

void DialogueRunner::emitEvent(const DialogueEvent& event) {
    if (eventCallback) {
        eventCallback(event);
    }
}

void DialogueRunner::updateAvailableTopics() {
    availableTopicFormIDs.clear();
    availableTopicNames.clear();

    if (!currentTree) return;

    // Get all topic groups
    auto allGroups = currentTree->getAllTopicGroups();

    for (const auto* group : allGroups) {
        // Skip greeting and farewell categories (handled separately)
        if (group->category == TopicCategory::Greeting ||
            group->category == TopicCategory::Farewell) {
            continue;
        }

        // Check if any node in this topic is available
        bool hasAvailable = false;
        for (uint32_t nodeID : group->nodeIDs) {
            const DialogueNode* node = currentTree->getNode(nodeID);
            if (node && node->isAvailable()) {
                // Quick filter check
                if (filterEngine) {
                    FilterResult result = filterEngine->evaluateNode(*node);
                    if (result.passed) {
                        hasAvailable = true;
                        break;
                    }
                } else {
                    hasAvailable = true;
                    break;
                }
            }
        }

        if (hasAvailable) {
            availableTopicFormIDs.push_back(group->topicFormID);
            availableTopicNames.push_back(group->topicName);
        }
    }

    DR_LOGD("Updated available topics: %zu", availableTopicFormIDs.size());
}

void DialogueRunner::executeResultScript(uint32_t scriptFormID) {
    if (!scriptManager || scriptFormID == 0) return;

    DR_LOGI("Executing result script 0x%08X", scriptFormID);

    // Start the script with NPC as self, player as target
    int result = scriptManager->startScript(scriptFormID, currentNPCFormID, 1);
    if (result >= 0) {
        scriptRunning = true;
        scriptTimer = 0.0f;

        DialogueEvent event;
        event.type = DialogueEvent::Type::ScriptResult;
        event.text = "Script started";
        event.success = true;
        emitEvent(event);
    } else {
        DR_LOGE("Failed to start script 0x%08X", scriptFormID);

        DialogueEvent event;
        event.type = DialogueEvent::Type::ScriptResult;
        event.text = "Script failed to start";
        event.success = false;
        emitEvent(event);
    }
}

void DialogueRunner::applySpeechcraftBonus(float amount) {
    if (!player) return;

    // Increase Speechcraft skill
    // In Oblivion, successful persuasion attempts increase Speechcraft
    DR_LOGD("Applying Speechcraft bonus: %.1f", amount);

    // The actual skill increase would be handled by the skill system
    // For now, emit a skill check event
    DialogueEvent event;
    event.type = DialogueEvent::Type::SkillCheck;
    event.text = "Speechcraft +" + std::to_string(static_cast<int>(amount));
    event.success = true;
    emitEvent(event);
}

bool DialogueRunner::matchTopicFromText(const std::string& input) const {
    if (!currentTree) return false;

    // Convert input to lowercase for matching
    std::string lowerInput = input;
    std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // Try to match against available topic names
    for (size_t i = 0; i < availableTopicNames.size(); ++i) {
        std::string lowerTopic = availableTopicNames[i];
        std::transform(lowerTopic.begin(), lowerTopic.end(), lowerTopic.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (lowerTopic.find(lowerInput) != std::string::npos ||
            lowerInput.find(lowerTopic) != std::string::npos) {
            // Found a match - select this topic
            const_cast<DialogueRunner*>(this)->selectTopic(static_cast<int>(i));
            return true;
        }
    }

    // Try partial matching on node prompt/response text
    auto allGroups = currentTree->getAllTopicGroups();
    for (const auto* group : allGroups) {
        for (uint32_t nodeID : group->nodeIDs) {
            const DialogueNode* node = currentTree->getNode(nodeID);
            if (!node) continue;

            std::string lowerPrompt = node->promptText;
            std::transform(lowerPrompt.begin(), lowerPrompt.end(), lowerPrompt.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (lowerPrompt.find(lowerInput) != std::string::npos) {
                const_cast<DialogueRunner*>(this)->selectTopicByFormID(group->topicFormID);
                return true;
            }
        }
    }

    return false;
}

void DialogueRunner::transitionToState(DialogueState newState) {
    DR_LOGD("State transition: %d -> %d", static_cast<int>(state), static_cast<int>(newState));
    state = newState;
}

} // namespace dialogue
} // namespace oblivion
