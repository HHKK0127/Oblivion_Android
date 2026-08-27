#pragma once

#include "script_vm.h"
#include "script_context.h"
#include "script_functions.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <android/log.h>

// ============================================================================
// Oblivion Script VM - Script Manager
// Loads SCPT records, manages VMs, runs scripts per frame
// ============================================================================

#define SCM_LOG_TAG "ScriptManager"
#ifdef ENABLE_DEBUG_LOGS
#define SCM_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, SCM_LOG_TAG, __VA_ARGS__)
#else
#define SCM_LOGD(...) do {} while(0)
#endif
#define SCM_LOGI(...) __android_log_print(ANDROID_LOG_INFO, SCM_LOG_TAG, __VA_ARGS__)
#define SCM_LOGW(...) __android_log_print(ANDROID_LOG_WARN, SCM_LOG_TAG, __VA_ARGS__)
#define SCM_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SCM_LOG_TAG, __VA_ARGS__)

// Forward declarations
class QuestManager;
class WorldManager;
class NpcManager;
class InventoryManager;

namespace oblivion {
namespace script {

// ============================================================================
// Active script instance
// ============================================================================
struct ActiveScript {
    uint32_t scriptFormID = 0;          // SCPT record FormID
    uint32_t selfRefFormID = 0;         // Object this script is attached to
    ExecutionContext context;            // Execution state
    bool waitingForFrame = false;       // True if hit frame budget last tick
    float waitTimer = 0.0f;             // For Wait() function
};

// ============================================================================
// ScriptManager - loads and manages all scripts
// ============================================================================
class ScriptManager {
public:
    ScriptManager();
    ~ScriptManager() = default;

    // Initialize with game systems
    void init(
        QuestManager* questMgr,
        WorldManager* worldMgr,
        NpcManager* npcMgr,
        InventoryManager* invMgr
    );

    // Load scripts from parsed ESM data
    void loadScripts(const std::vector<ScriptData>& scripts);

    // Add a single script
    void addScript(const ScriptData& script);

    // Start executing a script on an object
    // Returns the active script index, or -1 on failure
    int startScript(uint32_t scriptFormID, uint32_t selfRefFormID, uint32_t targetRefFormID = 0);

    // Stop a running script
    void stopScript(uint32_t scriptFormID, uint32_t selfRefFormID);

    // Stop all scripts for a given object
    void stopScriptsForObject(uint32_t selfRefFormID);

    // Update all active scripts (call once per frame)
    void update(float deltaTime);

    // Query
    const ScriptData* getScript(uint32_t formID) const;
    bool isScriptRunning(uint32_t scriptFormID, uint32_t selfRefFormID) const;
    size_t getActiveScriptCount() const { return activeScripts_.size(); }
    size_t getLoadedScriptCount() const { return scripts_.size(); }

    // Get the VM (for debugging)
    const ScriptVM& getVM() const { return vm_; }

    // Get global variables
    ScriptValue getGlobalVariable(uint32_t formID) const;
    void setGlobalVariable(uint32_t formID, const ScriptValue& value);

private:
    // Script storage (FormID -> ScriptData)
    std::unordered_map<uint32_t, ScriptData> scripts_;

    // Active script instances
    std::vector<ActiveScript> activeScripts_;

    // Global variables (FormID -> value)
    std::unordered_map<uint32_t, ScriptValue> globalVariables_;

    // VM and function registry
    ScriptVM vm_;
    ScriptFunctions functions_;

    // Game system pointers
    QuestManager* questManager_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;

    // Find active script by formID + selfRef
    ActiveScript* findActiveScript(uint32_t scriptFormID, uint32_t selfRefFormID);
};

} // namespace script
} // namespace oblivion
