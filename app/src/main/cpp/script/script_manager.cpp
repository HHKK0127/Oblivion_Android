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
    InventoryManager* invMgr
) {
    questManager_ = questMgr;
    worldManager_ = worldMgr;
    npcManager_ = npcMgr;
    inventoryManager_ = invMgr;

    functions_.init(questMgr, worldMgr, npcMgr, invMgr);

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

void ScriptManager::stopScript(uint32_t scriptFormID, uint32_t selfRefFormID) {
    auto it = std::remove_if(activeScripts_.begin(), activeScripts_.end(),
        [&](const ActiveScript& s) {
            return s.scriptFormID == scriptFormID && s.selfRefFormID == selfRefFormID;
        });

    if (it != activeScripts_.end()) {
        activeScripts_.erase(it, activeScripts_.end());
        SCM_LOGD("Stopped script 0x%08X on object 0x%08X", scriptFormID, selfRefFormID);
    }
}

void ScriptManager::stopScriptsForObject(uint32_t selfRefFormID) {
    auto it = std::remove_if(activeScripts_.begin(), activeScripts_.end(),
        [&](const ActiveScript& s) {
            return s.selfRefFormID == selfRefFormID;
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
                activeScripts_.erase(activeScripts_.begin() + i);
                break;

            case VMResult::NotRunning:
                // Shouldn't happen, but remove it
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
        if (active.scriptFormID == scriptFormID && active.selfRefFormID == selfRefFormID) {
            return true;
        }
    }
    return false;
}

ActiveScript* ScriptManager::findActiveScript(uint32_t scriptFormID, uint32_t selfRefFormID) {
    for (auto& active : activeScripts_) {
        if (active.scriptFormID == scriptFormID && active.selfRefFormID == selfRefFormID) {
            return &active;
        }
    }
    return nullptr;
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
