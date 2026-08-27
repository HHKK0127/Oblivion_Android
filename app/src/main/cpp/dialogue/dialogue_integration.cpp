#include "dialogue_integration.h"
#include <algorithm>

namespace oblivion {
namespace dialogue {

// ============================================================================
// DialogueIntegration
// ============================================================================

DialogueIntegration::DialogueIntegration() {
    // Wire up the runner's event callback
    runner.setEventCallback([this](const DialogueEvent& event) {
        handleDialogueEvent(event);
    });
}

DialogueIntegration::~DialogueIntegration() = default;

bool DialogueIntegration::initialize(
    NpcManager* npcMgr,
    QuestManager* questMgr,
    oblivion::FactionManager* factionMgr,
    oblivion::script::ScriptManager* scriptMgr,
    Player* player) {

    npcManager = npcMgr;
    questManager = questMgr;
    factionManager = factionMgr;
    scriptManager = scriptMgr;
    this->player = player;

    // Wire up subsystems
    filterEngine.setQuestManager(questMgr);
    filterEngine.setFactionManager(factionMgr);
    runner.setFilterEngine(&filterEngine);
    runner.setHistory(&history);
    runner.setScriptManager(scriptMgr);
    runner.setNpcManager(npcMgr);
    runner.setPlayer(player);

    DI_LOGI("DialogueIntegration initialized");
    return true;
}

void DialogueIntegration::loadFromESM(const oblivion::ESMManager& esmMgr) {
    DI_LOGI("Loading dialogue data from ESM...");

    // Get all DIAL and INFO records from ESM
    const auto& esmDialogs = esmMgr.getAllDialogs();

    // Convert ESM dialog data to DialogueDialRecords
    std::vector<DialogueDialRecord> dialRecords;

    for (const auto& esmDialog : esmDialogs) {
        if (esmDialog.formID == 0) continue;

        DialogueDialRecord dial;
        dial.formID = esmDialog.formID;
        dial.editorID = esmDialog.editorID;
        dial.fullName = esmDialog.fullName;
        dial.type = static_cast<DialogueType>(esmDialog.dialogType);
        dial.flags = esmDialog.flags;

        // Convert INFO records
        for (const auto& esmInfo : esmDialog.infos) {
            DialogueInfoRecord info;
            info.formID = esmInfo.formID;
            info.editorID = esmInfo.editorID;
            info.dialFormID = esmInfo.dialFormID;
            info.responseText = esmInfo.responseText;
            info.promptText = esmInfo.promptText;
            info.dialogType = esmInfo.responseType;
            info.flags = esmInfo.flags;
            info.factionFormID = esmInfo.factionFormID;
            info.factionRank = esmInfo.factionRank;
            info.questFormID = esmInfo.questFormID;
            info.questStage = esmInfo.questStage;

            // Create a basic response from the info data
            ResponseData resp;
            resp.type = static_cast<ResponseType>(esmInfo.responseType);
            resp.responseText = esmInfo.responseText;
            resp.promptText = esmInfo.promptText;
            info.responses.push_back(resp);

            // Set priority based on specificity
            info.priority = 0;
            if (info.factionFormID != 0) info.priority += 20;
            if (info.questFormID != 0) info.priority += 50;
            if (info.questStage >= 0) info.priority += 30;

            dial.infos.push_back(std::move(info));
        }

        dialRecords.push_back(std::move(dial));
    }

    // Build trees for all NPCs that have dialogue
    // In Oblivion, DIAL records are global topics, not per-NPC
    // We create trees per NPC by filtering based on conditions
    // For now, create a shared tree structure

    // Group by NPC FormID from conditions
    std::unordered_map<uint32_t, std::vector<DialogueDialRecord>> npcDialogues;

    for (const auto& dial : dialRecords) {
        // Check if any INFO has an NPC filter
        bool hasNPCFilter = false;
        for (const auto& info : dial.infos) {
            if (info.filterNPCFormID != 0) {
                npcDialogues[info.filterNPCFormID].push_back(dial);
                hasNPCFilter = true;
            }
        }

        // If no NPC filter, this is a global topic available to all NPCs
        if (!hasNPCFilter) {
            // Add to a "global" bucket (FormID 0)
            npcDialogues[0].push_back(dial);
        }
    }

    // Build trees
    for (const auto& [npcFormID, dials] : npcDialogues) {
        if (npcFormID == 0) continue; // Skip global for now

        auto tree = std::make_unique<DialogueTree>();
        tree->buildFromDialRecords(dials, npcFormID);
        trees[npcFormID] = std::move(tree);
    }

    // Also create a global tree for NPC-unfiltered dialogues
    if (npcDialogues.count(0) > 0) {
        auto globalTree = std::make_unique<DialogueTree>();
        globalTree->buildFromDialRecords(npcDialogues[0], 0);
        trees[0] = std::move(globalTree);
    }

    DI_LOGI("Loaded %zu dialogue trees from %zu DIAL records",
            trees.size(), dialRecords.size());
}

bool DialogueIntegration::startDialogueWithNPC(uint32_t npcFormID) {
    DI_LOGI("Starting dialogue with NPC 0x%08X", npcFormID);

    // Find the dialogue tree for this NPC
    DialogueTree* tree = getTreeForNPC(npcFormID);
    if (!tree) {
        DI_LOGW("No dialogue tree found for NPC 0x%08X", npcFormID);
        return false;
    }

    // Update filter context
    updateFilterContext(npcFormID);

    // Start dialogue
    return runner.startDialogue(npcFormID, *tree);
}

bool DialogueIntegration::startDialogueWithNearestNPC(const glm::vec3& playerPos, float maxDistance) {
    uint32_t npcFormID = findNearestDialogueNPC(playerPos, maxDistance);
    if (npcFormID == 0) {
        DI_LOGD("No NPC with dialogue found within range");
        return false;
    }

    return startDialogueWithNPC(npcFormID);
}

void DialogueIntegration::endDialogue() {
    runner.endDialogue();
}

bool DialogueIntegration::selectTopic(int topicIndex) {
    return runner.selectTopic(topicIndex);
}

bool DialogueIntegration::selectChoice(int choiceIndex) {
    return runner.selectChoice(choiceIndex);
}

bool DialogueIntegration::processTextInput(const std::string& input) {
    return runner.processTextInput(input);
}

void DialogueIntegration::update(float deltaTime) {
    runner.update(deltaTime);

    // Update game time in history
    history.setGameTime(history.getGameTime() + static_cast<uint32_t>(deltaTime));
}

bool DialogueIntegration::isDialogueActive() const {
    return runner.isDialogueActive();
}

DialogueTree* DialogueIntegration::getTreeForNPC(uint32_t npcFormID) {
    // Try NPC-specific tree first
    auto it = trees.find(npcFormID);
    if (it != trees.end()) {
        return it->second.get();
    }

    // Fall back to global tree
    auto globalIt = trees.find(0);
    if (globalIt != trees.end()) {
        return globalIt->second.get();
    }

    return nullptr;
}

bool DialogueIntegration::hasDialogue(uint32_t npcFormID) const {
    return trees.count(npcFormID) > 0 || trees.count(0) > 0;
}

std::vector<uint32_t> DialogueIntegration::getAllDialogueNPCs() const {
    std::vector<uint32_t> result;
    result.reserve(trees.size());
    for (const auto& [fid, tree] : trees) {
        if (fid != 0) { // Exclude global tree
            result.push_back(fid);
        }
    }
    return result;
}

size_t DialogueIntegration::getTotalNodeCount() const {
    size_t total = 0;
    for (const auto& [fid, tree] : trees) {
        total += tree->getNodeCount();
    }
    return total;
}

void DialogueIntegration::logStats() const {
    DI_LOGI("===== DialogueIntegration Stats =====");
    DI_LOGI("Trees loaded: %zu", trees.size());
    DI_LOGI("Total nodes: %zu", getTotalNodeCount());
    DI_LOGI("History entries: %zu", history.getTotalEntryCount());
    DI_LOGI("NPCs tracked: %zu", history.getNPCCount());
    DI_LOGI("Active dialogue: %s", isDialogueActive() ? "YES" : "NO");
    DI_LOGI("=====================================");
}

// ============================================================================
// Private helpers
// ============================================================================

void DialogueIntegration::buildTreeForNPC(uint32_t npcFormID,
                                            const std::vector<DialogueDialRecord>& dialRecords) {
    auto tree = std::make_unique<DialogueTree>();
    tree->buildFromDialRecords(dialRecords, npcFormID);
    trees[npcFormID] = std::move(tree);
}

void DialogueIntegration::updateFilterContext(uint32_t npcFormID) {
    // Update player context
    if (player) {
        filterEngine.updatePlayerFromPlayer(*player, 1);
    }

    // Update NPC context
    if (npcManager) {
        auto npc = npcManager->getNPC(npcFormID);
        if (npc) {
            filterEngine.updateNPCFromNPC(*npc, npcFormID);
        }
    }

    // Update quest stages
    if (questManager) {
        oblivion::dialogue::PlayerContext ctx = filterEngine.getPlayerContext();
        auto activeQuests = questManager->getActiveQuests();
        for (const auto& quest : activeQuests) {
            // Map quest state to stage number
            int32_t stage = 0;
            switch (quest->state) {
                case QuestState::PENDING: stage = 0; break;
                case QuestState::ACCEPTED: stage = 10; break;
                case QuestState::IN_PROGRESS: stage = 50; break;
                case QuestState::COMPLETED: stage = 100; break;
                case QuestState::FAILED: stage = -1; break;
            }
            ctx.questStages[quest->questId] = stage;
        }
        filterEngine.setPlayerContext(ctx);
    }

    // Update faction memberships
    if (factionManager && player) {
        oblivion::dialogue::PlayerContext ctx = filterEngine.getPlayerContext();
        ctx.factionMemberships = factionManager->getNPCFactions(1); // Player FormID = 1
        filterEngine.setPlayerContext(ctx);
    }
}

void DialogueIntegration::handleDialogueEvent(const DialogueEvent& event) {
    // Forward to UI callback
    if (uiCallback) {
        uiCallback(event);
    }

    // Handle quest-related events
    if (event.type == DialogueEvent::Type::ScriptResult && event.success) {
        DI_LOGD("Script executed successfully");
    }
}

uint32_t DialogueIntegration::findNearestDialogueNPC(const glm::vec3& playerPos,
                                                        float maxDistance) const {
    if (!npcManager) return 0;

    uint32_t nearestNPC = 0;
    float nearestDist = maxDistance * maxDistance;

    auto nearbyNPCs = npcManager->getNPCsInArea(playerPos, maxDistance);
    for (const auto& npc : nearbyNPCs) {
        if (!hasDialogue(npc->npcId)) continue;

        float dx = npc->position.x - playerPos.x;
        float dy = npc->position.y - playerPos.y;
        float dz = npc->position.z - playerPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < nearestDist) {
            nearestDist = distSq;
            nearestNPC = npc->npcId;
        }
    }

    return nearestNPC;
}

} // namespace dialogue
} // namespace oblivion
