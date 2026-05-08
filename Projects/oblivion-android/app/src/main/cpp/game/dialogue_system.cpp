#include "dialogue_system.h"
#include "npc.h"
#include <stdexcept>

DialogueSystem::DialogueSystem() {
    LOGD("DialogueSystem created");
}

DialogueSystem::~DialogueSystem() {
    LOGD("DialogueSystem destroyed");
}

void DialogueSystem::registerDialogueTree(const DialogueTree& tree) {
    try {
        uint32_t treeId = std::stoul(tree.treeId);
        dialogueTrees[treeId] = tree;
        LOGD("Dialogue tree registered: %s (%zu nodes)",
             tree.treeId.c_str(), tree.nodes.size());
    } catch (const std::exception& e) {
        LOGE("Failed to register dialogue tree: Invalid treeId '%s' - %s",
             tree.treeId.c_str(), e.what());
    }
}

const DialogueTree* DialogueSystem::getDialogueTree(uint32_t npcId) const {
    auto it = dialogueTrees.find(npcId);
    if (it != dialogueTrees.end()) {
        return &it->second;
    }
    return nullptr;
}

const DialogueNode* DialogueSystem::getDialogueNode(uint32_t npcId,
                                                     const std::string& nodeId) const {
    const auto* tree = getDialogueTree(npcId);
    if (!tree) return nullptr;

    auto it = tree->nodes.find(nodeId);
    if (it != tree->nodes.end()) {
        return &it->second;
    }
    return nullptr;
}

DialogueSystem::ConversationState* DialogueSystem::findOrCreateConversation(uint32_t npcId) {
    auto it = activeConversations.find(npcId);
    if (it != activeConversations.end()) {
        return &it->second;
    }

    ConversationState newConv;
    newConv.npcId = npcId;
    newConv.isActive = false;
    newConv.conversationTime = 0.0f;

    activeConversations[npcId] = newConv;
    return &activeConversations[npcId];
}

bool DialogueSystem::startConversation(std::shared_ptr<NPC> npc) {
    if (!npc) {
        LOGW("startConversation called with null NPC");
        return false;
    }

    const auto* tree = getDialogueTree(npc->npcId);
    if (!tree) {
        LOGW("No dialogue tree found for NPC %u", npc->npcId);
        return false;
    }

    auto* convState = findOrCreateConversation(npc->npcId);
    convState->npcId = npc->npcId;
    convState->currentNodeId = tree->rootNodeId;
    convState->isActive = true;
    convState->conversationTime = 0.0f;

    LOGI("Conversation started with NPC %u (name: %s)",
         npc->npcId, tree->npcName.c_str());

    return true;
}

bool DialogueSystem::selectOption(uint32_t npcId, int optionIndex) {
    auto* convState = findOrCreateConversation(npcId);
    if (!convState || !convState->isActive) {
        LOGW("No active conversation for NPC %u", npcId);
        return false;
    }

    const auto* currentNode = getDialogueNode(npcId, convState->currentNodeId);
    if (!currentNode) {
        LOGW("Current dialogue node not found for NPC %u", npcId);
        return false;
    }

    // Check if option index is valid
    if (optionIndex < 0 || optionIndex >= static_cast<int>(currentNode->playerOptionKeys.size())) {
        int maxValidIndex = (currentNode->playerOptionKeys.size() > 0)
            ? static_cast<int>(currentNode->playerOptionKeys.size() - 1)
            : -1;
        LOGW("Invalid option index %d for NPC %u (max valid: %d)",
             optionIndex, npcId, maxValidIndex);
        return false;
    }

    // Check if there's a next node for this option
    auto nextIt = currentNode->nextNodes.find(optionIndex);
    if (nextIt == currentNode->nextNodes.end()) {
        LOGW("No next node defined for option %d", optionIndex);
        return false;
    }

    // Move to next node
    convState->currentNodeId = nextIt->second;
    convState->conversationTime = 0.0f;

    LOGD("NPC %u: Selected option %d -> node %s",
         npcId, optionIndex, nextIt->second.c_str());

    // Check if next node ends dialogue
    const auto* nextNode = getDialogueNode(npcId, nextIt->second);
    if (nextNode && nextNode->endsDialogue) {
        endConversation(npcId);
    }

    return true;
}

bool DialogueSystem::endConversation(uint32_t npcId) {
    auto it = activeConversations.find(npcId);
    if (it != activeConversations.end()) {
        it->second.isActive = false;
        LOGI("Conversation ended with NPC %u", npcId);
        return true;
    }

    return false;
}

bool DialogueSystem::isConversationActive(uint32_t npcId) const {
    auto it = activeConversations.find(npcId);
    if (it != activeConversations.end()) {
        return it->second.isActive;
    }
    return false;
}

const DialogueNode* DialogueSystem::getCurrentNode(uint32_t npcId) const {
    auto it = activeConversations.find(npcId);
    if (it != activeConversations.end() && it->second.isActive) {
        return getDialogueNode(npcId, it->second.currentNodeId);
    }
    return nullptr;
}

std::string DialogueSystem::getCurrentNodeSpeech(uint32_t npcId) const {
    const auto* node = getCurrentNode(npcId);
    if (node) {
        return node->npcSpeakKey;  // Return localization key
    }
    return "";
}

std::vector<std::string> DialogueSystem::getCurrentOptions(uint32_t npcId) const {
    const auto* node = getCurrentNode(npcId);
    if (node) {
        return node->playerOptionKeys;
    }
    return {};
}

void DialogueSystem::initializeDialogues() {
    LOGI("Initializing dialogue trees");

    initializeCompanionDialogue();
    initializeMerchantDialogue();
    initializeGuardDialogue();
    initializeMageDialogue();

    LOGI("Dialogue trees initialized: %zu trees", dialogueTrees.size());
}

// ============================================================
// Dialogue Tree Initialization - Companion
// ============================================================
void DialogueSystem::initializeCompanionDialogue() {
    DialogueTree companion;
    companion.treeId = "1000";  // Companion NPC ID
    companion.npcName = "Companion";
    companion.rootNodeId = "greeting";

    // Root node: Greeting
    {
        DialogueNode greeting;
        greeting.nodeId = "greeting";
        greeting.npcSpeakKey = "dialogue_companion_greeting";
        greeting.playerOptionKeys = {
            "dialogue_option_quest",
            "dialogue_option_followme",
            "dialogue_option_goodbye"
        };
        greeting.nextNodes = {
            {0, "quest_offer"},
            {1, "follow_response"},
            {2, "goodbye"}
        };

        companion.nodes["greeting"] = greeting;
    }

    // Quest offer node
    {
        DialogueNode questOffer;
        questOffer.nodeId = "quest_offer";
        questOffer.npcSpeakKey = "dialogue_companion_quest";
        questOffer.playerOptionKeys = {
            "dialogue_option_accept",
            "dialogue_option_decline"
        };
        questOffer.nextNodes = {
            {0, "quest_accepted"},
            {1, "quest_declined"}
        };
        questOffer.questRewards[0] = 101;  // Quest ID 101

        companion.nodes["quest_offer"] = questOffer;
    }

    // Quest accepted
    {
        DialogueNode accepted;
        accepted.nodeId = "quest_accepted";
        accepted.npcSpeakKey = "dialogue_companion_accepted";
        accepted.playerOptionKeys = {"dialogue_option_bye"};
        accepted.nextNodes = {{0, "goodbye"}};
        accepted.endsDialogue = false;

        companion.nodes["quest_accepted"] = accepted;
    }

    // Quest declined
    {
        DialogueNode declined;
        declined.nodeId = "quest_declined";
        declined.npcSpeakKey = "dialogue_companion_declined";
        declined.playerOptionKeys = {"dialogue_option_bye"};
        declined.nextNodes = {{0, "goodbye"}};
        declined.endsDialogue = false;

        companion.nodes["quest_declined"] = declined;
    }

    // Follow response
    {
        DialogueNode follow;
        follow.nodeId = "follow_response";
        follow.npcSpeakKey = "dialogue_companion_follow";
        follow.playerOptionKeys = {"dialogue_option_bye"};
        follow.nextNodes = {{0, "goodbye"}};

        companion.nodes["follow_response"] = follow;
    }

    // Goodbye
    {
        DialogueNode goodbye;
        goodbye.nodeId = "goodbye";
        goodbye.npcSpeakKey = "dialogue_companion_goodbye";
        goodbye.playerOptionKeys = {};
        goodbye.nextNodes = {};
        goodbye.endsDialogue = true;

        companion.nodes["goodbye"] = goodbye;
    }

    registerDialogueTree(companion);
}

// ============================================================
// Dialogue Tree Initialization - Merchant
// ============================================================
void DialogueSystem::initializeMerchantDialogue() {
    DialogueTree merchant;
    merchant.treeId = "2000";  // Merchant NPC ID
    merchant.npcName = "Merchant";
    merchant.rootNodeId = "greeting";

    // Greeting
    {
        DialogueNode greeting;
        greeting.nodeId = "greeting";
        greeting.npcSpeakKey = "dialogue_merchant_greeting";
        greeting.playerOptionKeys = {
            "dialogue_option_trade",
            "dialogue_option_news",
            "dialogue_option_goodbye"
        };
        greeting.nextNodes = {
            {0, "trade_response"},
            {1, "news_response"},
            {2, "goodbye"}
        };

        merchant.nodes["greeting"] = greeting;
    }

    // Trade response
    {
        DialogueNode trade;
        trade.nodeId = "trade_response";
        trade.npcSpeakKey = "dialogue_merchant_trade";
        trade.playerOptionKeys = {"dialogue_option_bye"};
        trade.nextNodes = {{0, "goodbye"}};

        merchant.nodes["trade_response"] = trade;
    }

    // News response
    {
        DialogueNode news;
        news.nodeId = "news_response";
        news.npcSpeakKey = "dialogue_merchant_news";
        news.playerOptionKeys = {"dialogue_option_bye"};
        news.nextNodes = {{0, "goodbye"}};

        merchant.nodes["news_response"] = news;
    }

    // Goodbye
    {
        DialogueNode goodbye;
        goodbye.nodeId = "goodbye";
        goodbye.npcSpeakKey = "dialogue_merchant_goodbye";
        goodbye.playerOptionKeys = {};
        goodbye.nextNodes = {};
        goodbye.endsDialogue = true;

        merchant.nodes["goodbye"] = goodbye;
    }

    registerDialogueTree(merchant);
}

// ============================================================
// Dialogue Tree Initialization - Guard
// ============================================================
void DialogueSystem::initializeGuardDialogue() {
    DialogueTree guard;
    guard.treeId = "3000";  // Guard NPC ID
    guard.npcName = "Guard";
    guard.rootNodeId = "greeting";

    // Greeting
    {
        DialogueNode greeting;
        greeting.nodeId = "greeting";
        greeting.npcSpeakKey = "dialogue_guard_greeting";
        greeting.playerOptionKeys = {
            "dialogue_option_help",
            "dialogue_option_news",
            "dialogue_option_goodbye"
        };
        greeting.nextNodes = {
            {0, "help_response"},
            {1, "news_response"},
            {2, "goodbye"}
        };

        guard.nodes["greeting"] = greeting;
    }

    // Help response
    {
        DialogueNode help;
        help.nodeId = "help_response";
        help.npcSpeakKey = "dialogue_guard_help";
        help.playerOptionKeys = {"dialogue_option_bye"};
        help.nextNodes = {{0, "goodbye"}};

        guard.nodes["help_response"] = help;
    }

    // News
    {
        DialogueNode news;
        news.nodeId = "news_response";
        news.npcSpeakKey = "dialogue_guard_news";
        news.playerOptionKeys = {"dialogue_option_bye"};
        news.nextNodes = {{0, "goodbye"}};

        guard.nodes["news_response"] = news;
    }

    // Goodbye
    {
        DialogueNode goodbye;
        goodbye.nodeId = "goodbye";
        goodbye.npcSpeakKey = "dialogue_guard_goodbye";
        goodbye.playerOptionKeys = {};
        goodbye.nextNodes = {};
        goodbye.endsDialogue = true;

        guard.nodes["goodbye"] = goodbye;
    }

    registerDialogueTree(guard);
}

// ============================================================
// Dialogue Tree Initialization - Mage
// ============================================================
void DialogueSystem::initializeMageDialogue() {
    DialogueTree mage;
    mage.treeId = "4000";  // Mage NPC ID
    mage.npcName = "Mage";
    mage.rootNodeId = "greeting";

    // Greeting
    {
        DialogueNode greeting;
        greeting.nodeId = "greeting";
        greeting.npcSpeakKey = "dialogue_mage_greeting";
        greeting.playerOptionKeys = {
            "dialogue_option_spells",
            "dialogue_option_magic",
            "dialogue_option_goodbye"
        };
        greeting.nextNodes = {
            {0, "spells_response"},
            {1, "magic_response"},
            {2, "goodbye"}
        };

        mage.nodes["greeting"] = greeting;
    }

    // Spells
    {
        DialogueNode spells;
        spells.nodeId = "spells_response";
        spells.npcSpeakKey = "dialogue_mage_spells";
        spells.playerOptionKeys = {"dialogue_option_bye"};
        spells.nextNodes = {{0, "goodbye"}};

        mage.nodes["spells_response"] = spells;
    }

    // Magic
    {
        DialogueNode magic;
        magic.nodeId = "magic_response";
        magic.npcSpeakKey = "dialogue_mage_magic";
        magic.playerOptionKeys = {"dialogue_option_bye"};
        magic.nextNodes = {{0, "goodbye"}};

        mage.nodes["magic_response"] = magic;
    }

    // Goodbye
    {
        DialogueNode goodbye;
        goodbye.nodeId = "goodbye";
        goodbye.npcSpeakKey = "dialogue_mage_goodbye";
        goodbye.playerOptionKeys = {};
        goodbye.nextNodes = {};
        goodbye.endsDialogue = true;

        mage.nodes["goodbye"] = goodbye;
    }

    registerDialogueTree(mage);
}
