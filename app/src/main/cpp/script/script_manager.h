#pragma once

#include "script_vm.h"
#include "script_context.h"
#include "script_functions.h"
#include "native_scda_vm.h"
#include "native_scda_bridge.h"
#include "native_scda_decoder.h"
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
class QuestFlowController;
class WorldManager;
class NpcManager;
class InventoryManager;

namespace oblivion {
namespace script {

struct InlineScriptKey {
    uint32_t questFormID = 0;
    uint16_t stageIndex = 0;
    uint16_t scriptIndex = 0;
};

// ============================================================================
// Active script instance
// ============================================================================
struct ActiveScript {
    uint32_t scriptFormID = 0;          // SCPT record FormID
    uint32_t selfRefFormID = 0;         // Object this script is attached to
    InlineScriptKey inlineKey;
    int32_t inlineScriptIndex = -1;
    bool isInlineScript = false;
    ExecutionContext context;            // Execution state for synthetic VM
    bool waitingForFrame = false;       // True if hit frame budget last tick
    bool paused = false;                // True while stopped on a breakpoint
    float waitTimer = 0.0f;             // For Wait() function
    // Native SCDA VM state. Valid when nativeProgram is non-null.
    const NativeDecodeResult* nativeProgram = nullptr;
    NativeVariableStore nativeVarStore;
    bool nativeRunning = false;
    bool nativeFrameBudget = false;
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
        InventoryManager* invMgr,
        QuestFlowController* questFlowController = nullptr
    );

    // Load scripts from parsed ESM data
    void loadScripts(const std::vector<ScriptData>& scripts);

    // Add a single script
    void addScript(const ScriptData& script);

    // Start executing a script on an object
    // Returns the active script index, or -1 on failure
    int startScript(uint32_t scriptFormID, uint32_t selfRefFormID, uint32_t targetRefFormID = 0);

    // Start a script embedded in a quest stage. The manager retains its data
    // so ExecutionContext bytecode pointers remain valid while it runs.
    int startInlineScript(const ScriptData& script,
                          const InlineScriptKey& key,
                          uint32_t selfRefFormID,
                          uint32_t targetRefFormID = 0);

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

    // ========================================================================
    // Debugger interface (game console / DBG_SCRIPT tab)
    // Every method returns human-readable text and never throws. Script
    // selection is tracked so index -1 means "the selected script".
    // ========================================================================
    std::string debugListScripts() const;
    std::string debugScriptInfo(int index);
    std::string debugDisasm(int index, uint32_t startOffset, int count);
    std::string debugStepScript(int index);
    std::string debugResumeScript(int index);
    std::string debugToggleBreakpoint(int index, uint32_t offset);
    std::string debugListBreakpoints() const;
    std::string debugClearBreakpoints();
    std::string debugScriptVariables(int index);
    std::string debugListGlobals() const;
    std::string debugGetGlobal(uint32_t formID) const;
    std::string debugSetGlobal(uint32_t formID, const std::string& valueText);
    std::string debugRunScript(uint32_t formID, uint32_t selfRefFormID);

private:
    // Script storage (FormID -> ScriptData)
    std::unordered_map<uint32_t, ScriptData> scripts_;
    std::vector<std::unique_ptr<ScriptData>> inlineScripts_;

    // Active script instances
    std::vector<ActiveScript> activeScripts_;

    // Global variables (FormID -> value)
    std::unordered_map<uint32_t, ScriptValue> globalVariables_;

    // VM and function registry
    ScriptVM vm_;
    ScriptFunctions functions_;

    // Native SCDA VM for compiled quest/object scripts
    NativeScdaVm nativeVm_;
    std::unordered_map<uint32_t, NativeDecodeResult> nativePrograms_; // scriptFormID -> decoded program
    NativeDecodeResult decodeNativeScda(const ScriptData& script);

    // Game system pointers
    QuestManager* questManager_ = nullptr;
    QuestFlowController* questFlowController_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;

    // Find active script by formID + selfRef
    ActiveScript* findActiveScript(uint32_t scriptFormID, uint32_t selfRefFormID);
    bool isInlineScriptRunning(const InlineScriptKey& key, uint32_t selfRefFormID) const;
    void releaseInlineScript(const ActiveScript& active);

    // Debugger helpers
    int debugSelectedScript_ = -1;
    // Resolves -1 (selected) / explicit index. Returns -1 when no scripts
    // exist, -2 when an explicit index is out of range, else the index.
    int resolveDebugIndex(int index);
    void clampDebugSelection();
    static std::string describeScriptState(const ActiveScript& active);
    static std::string describeActiveScript(size_t index, const ActiveScript& active);
    std::string formatDisasmWindow(const ScriptData& script, uint32_t pc,
                                   int before, int after) const;
    std::string formatFault(const ActiveScript& active) const;
    std::string applyDebugStep(ActiveScript& active, size_t index, bool resume);

    // Native SCDA execution helpers
    void runSyntheticScript(ActiveScript& active, int index);
    void runNativeScript(ActiveScript& active, int index);
};

} // namespace script
} // namespace oblivion
