#include "script_manager.h"
#include <algorithm>

// ============================================================================
// Oblivion Script VM - Script Manager Implementation
// ============================================================================

namespace oblivion {
namespace script {

ScriptManager::ScriptManager() {
    vm_.setFunctions(&functions_);
}

void ScriptManager::init(
    QuestManager* questMgr,
    WorldManager* worldMgr,
    NpcManager* npcMgr,
    InventoryManager* invMgr,
    QuestFlowController* questFlowController
) {
    questManager_ = questMgr;
    questFlowController_ = questFlowController;
    worldManager_ = worldMgr;
    npcManager_ = npcMgr;
    inventoryManager_ = invMgr;

    functions_.init(questMgr, worldMgr, npcMgr, invMgr, questFlowController);

    SCM_LOGI("ScriptManager initialized");
}

// ============================================================================
// Script loading
// ============================================================================

void ScriptManager::loadScripts(const std::vector<ScriptData>& scripts) {
    for (const auto& script : scripts) {
        addScript(script);
    }
    SCM_LOGI("Loaded %zu scripts", scripts.size());
}

void ScriptManager::addScript(const ScriptData& script) {
    scripts_[script.formID] = script;
    SCM_LOGD("Added script 0x%08X: %s (type=%d, bytecode=%zu bytes)",
             script.formID, script.editorID.c_str(),
             static_cast<int>(script.scriptType),
             script.bytecode.size());
}

// ============================================================================
// Script execution control
// ============================================================================

int ScriptManager::startScript(uint32_t scriptFormID, uint32_t selfRefFormID, uint32_t targetRefFormID) {
    // Find script data
    auto it = scripts_.find(scriptFormID);
    if (it == scripts_.end()) {
        SCM_LOGW("Script 0x%08X not found", scriptFormID);
        return -1;
    }

    // Check if already running
    if (isScriptRunning(scriptFormID, selfRefFormID)) {
        SCM_LOGD("Script 0x%08X already running on 0x%08X", scriptFormID, selfRefFormID);
        return -1;
    }

    // Create active script
    ActiveScript active;
    active.scriptFormID = scriptFormID;
    active.selfRefFormID = selfRefFormID;
    active.context.init(&it->second);
    active.context.setSelfRef(selfRefFormID);
    active.context.setTargetRef(targetRefFormID);
    active.waitingForFrame = false;
    active.waitTimer = 0.0f;

    activeScripts_.push_back(std::move(active));

    SCM_LOGD("Started script 0x%08X on object 0x%08X", scriptFormID, selfRefFormID);

    return static_cast<int>(activeScripts_.size() - 1);
}

int ScriptManager::startInlineScript(
    const ScriptData& script,
    const InlineScriptKey& key,
    uint32_t selfRefFormID,
    uint32_t targetRefFormID
) {
    if (script.bytecode.empty()) {
        SCM_LOGW("Inline script for quest 0x%08X stage %u has no bytecode",
                 key.questFormID, key.stageIndex);
        return -1;
    }

    if (isInlineScriptRunning(key, selfRefFormID)) {
        SCM_LOGD("Inline script for quest 0x%08X stage %u index %u is already running",
                 key.questFormID, key.stageIndex, key.scriptIndex);
        return -1;
    }

    int32_t inlineScriptIndex = -1;
    for (size_t i = 0; i < inlineScripts_.size(); ++i) {
        if (!inlineScripts_[i]) {
            inlineScripts_[i] = std::make_unique<ScriptData>(script);
            inlineScriptIndex = static_cast<int32_t>(i);
            break;
        }
    }
    if (inlineScriptIndex < 0) {
        inlineScripts_.push_back(std::make_unique<ScriptData>(script));
        inlineScriptIndex = static_cast<int32_t>(inlineScripts_.size() - 1);
    }

    ActiveScript active;
    active.selfRefFormID = selfRefFormID;
    active.inlineKey = key;
    active.inlineScriptIndex = inlineScriptIndex;
    active.isInlineScript = true;
    active.context.init(inlineScripts_[inlineScriptIndex].get());
    active.context.setSelfRef(selfRefFormID);
    active.context.setTargetRef(targetRefFormID);

    activeScripts_.push_back(std::move(active));

    SCM_LOGD("Started inline script for quest 0x%08X stage %u index %u on 0x%08X",
             key.questFormID, key.stageIndex, key.scriptIndex, selfRefFormID);
    return static_cast<int>(activeScripts_.size() - 1);
}

void ScriptManager::stopScript(uint32_t scriptFormID, uint32_t selfRefFormID) {
    auto it = std::remove_if(activeScripts_.begin(), activeScripts_.end(),
        [&](const ActiveScript& s) {
            return !s.isInlineScript &&
                   s.scriptFormID == scriptFormID &&
                   s.selfRefFormID == selfRefFormID;
        });

    if (it != activeScripts_.end()) {
        activeScripts_.erase(it, activeScripts_.end());
        SCM_LOGD("Stopped script 0x%08X on object 0x%08X", scriptFormID, selfRefFormID);
    }
}

void ScriptManager::stopScriptsForObject(uint32_t selfRefFormID) {
    auto it = std::remove_if(activeScripts_.begin(), activeScripts_.end(),
        [&](const ActiveScript& s) {
            if (s.selfRefFormID == selfRefFormID) {
                releaseInlineScript(s);
                return true;
            }
            return false;
        });

    if (it != activeScripts_.end()) {
        size_t count = std::distance(it, activeScripts_.end());
        activeScripts_.erase(it, activeScripts_.end());
        SCM_LOGD("Stopped %zu scripts on object 0x%08X", count, selfRefFormID);
    }
}

// ============================================================================
// Frame update
// ============================================================================

void ScriptManager::update(float deltaTime) {
    // Process active scripts
    // Iterate in reverse so we can safely remove completed scripts
    for (int i = static_cast<int>(activeScripts_.size()) - 1; i >= 0; --i) {
        ActiveScript& active = activeScripts_[i];

        // Handle wait timer
        if (active.waitTimer > 0.0f) {
            active.waitTimer -= deltaTime;
            if (active.waitTimer > 0.0f) {
                continue;  // Still waiting
            }
            active.waitTimer = 0.0f;
        }

        // Execute script
        VMResult result = vm_.execute(active.context);

        switch (result) {
            case VMResult::Success:
                // Script completed (STOP opcode reached)
                SCM_LOGD("Script 0x%08X completed on 0x%08X",
                         active.scriptFormID, active.selfRefFormID);
                releaseInlineScript(active);
                activeScripts_.erase(activeScripts_.begin() + i);
                break;

            case VMResult::FrameBudget:
                // Script will continue next frame
                active.waitingForFrame = true;
                break;

            case VMResult::Error:
                SCM_LOGE("Script 0x%08X error on 0x%08X: %s",
                         active.scriptFormID, active.selfRefFormID,
                         vm_.getLastError().c_str());
                releaseInlineScript(active);
                activeScripts_.erase(activeScripts_.begin() + i);
                break;

            case VMResult::NotRunning:
                // Shouldn't happen, but remove it
                releaseInlineScript(active);
                activeScripts_.erase(activeScripts_.begin() + i);
                break;
        }
    }
}

// ============================================================================
// Query
// ============================================================================

const ScriptData* ScriptManager::getScript(uint32_t formID) const {
    auto it = scripts_.find(formID);
    if (it != scripts_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ScriptManager::isScriptRunning(uint32_t scriptFormID, uint32_t selfRefFormID) const {
    for (const auto& active : activeScripts_) {
        if (!active.isInlineScript &&
            active.scriptFormID == scriptFormID &&
            active.selfRefFormID == selfRefFormID) {
            return true;
        }
    }
    return false;
}

ActiveScript* ScriptManager::findActiveScript(uint32_t scriptFormID, uint32_t selfRefFormID) {
    for (auto& active : activeScripts_) {
        if (!active.isInlineScript &&
            active.scriptFormID == scriptFormID &&
            active.selfRefFormID == selfRefFormID) {
            return &active;
        }
    }
    return nullptr;
}

bool ScriptManager::isInlineScriptRunning(
    const InlineScriptKey& key, uint32_t selfRefFormID) const {
    for (const auto& active : activeScripts_) {
        if (active.isInlineScript &&
            active.selfRefFormID == selfRefFormID &&
            active.inlineKey.questFormID == key.questFormID &&
            active.inlineKey.stageIndex == key.stageIndex &&
            active.inlineKey.scriptIndex == key.scriptIndex) {
            return true;
        }
    }
    return false;
}

void ScriptManager::releaseInlineScript(const ActiveScript& active) {
    if (!active.isInlineScript || active.inlineScriptIndex < 0 ||
        static_cast<size_t>(active.inlineScriptIndex) >= inlineScripts_.size()) {
        return;
    }
    inlineScripts_[active.inlineScriptIndex].reset();
}

// ============================================================================
// Global variables
// ============================================================================

ScriptValue ScriptManager::getGlobalVariable(uint32_t formID) const {
    auto it = globalVariables_.find(formID);
    if (it != globalVariables_.end()) {
        return it->second;
    }
    return ScriptValue::makeInt(0);
}

void ScriptManager::setGlobalVariable(uint32_t formID, const ScriptValue& value) {
    globalVariables_[formID] = value;
}

} // namespace script
} // namespace oblivion
