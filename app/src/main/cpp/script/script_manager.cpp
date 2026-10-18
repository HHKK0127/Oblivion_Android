#include "script_manager.h"
#include "script_disasm.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <sstream>

// ============================================================================
// Oblivion Script VM - Script Manager Implementation
// ============================================================================

namespace oblivion {
namespace script {

namespace {

std::string hexU32(uint32_t value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%08X", value);
    return buf;
}

std::string hexOffset(uint32_t value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04X", value);
    return buf;
}

}  // namespace

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

        // Paused scripts (breakpoint stops) stay in the list untouched until
        // the debugger resumes or steps them.
        if (active.paused) {
            continue;
        }

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

            case VMResult::Breakpoint:
                // The instruction at the breakpoint did not run; PC still
                // points at it so the debugger can inspect before stepping.
                active.paused = true;
                active.waitingForFrame = false;
                SCM_LOGI("Script 0x%08X on 0x%08X hit breakpoint at PC=0x%X (paused)",
                         active.scriptFormID, active.selfRefFormID,
                         active.context.getPC());
                break;

            case VMResult::Error: {
                const std::string fault = formatFault(active);
                std::istringstream lines(fault);
                std::string line;
                while (std::getline(lines, line)) {
                    SCM_LOGE("%s", line.c_str());
                }
                releaseInlineScript(active);
                activeScripts_.erase(activeScripts_.begin() + i);
                break;
            }

            case VMResult::NotRunning:
                // Shouldn't happen, but remove it
                releaseInlineScript(active);
                activeScripts_.erase(activeScripts_.begin() + i);
                break;
        }
    }

    clampDebugSelection();
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
    // Drop breakpoints that point at this copy first: the slot is reused for
    // the next inline script, and a stale pointer would fire on it.
    const ScriptData* script = inlineScripts_[active.inlineScriptIndex].get();
    if (script) {
        vm_.removeBreakpointsFor(script);
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

// ============================================================================
// Debugger interface
// ============================================================================

int ScriptManager::resolveDebugIndex(int index) {
    if (activeScripts_.empty()) {
        debugSelectedScript_ = -1;
        return -1;
    }
    if (index >= 0) {
        if (static_cast<size_t>(index) >= activeScripts_.size()) {
            return -2;
        }
        return index;
    }
    if (debugSelectedScript_ >= 0 &&
        static_cast<size_t>(debugSelectedScript_) < activeScripts_.size()) {
        return debugSelectedScript_;
    }
    return 0;
}

void ScriptManager::clampDebugSelection() {
    if (debugSelectedScript_ >= static_cast<int>(activeScripts_.size())) {
        debugSelectedScript_ = static_cast<int>(activeScripts_.size()) - 1;
    }
}

std::string ScriptManager::describeScriptState(const ActiveScript& active) {
    if (!active.context.isRunning()) {
        return "stopped";
    }
    if (active.paused) {
        return "paused";
    }
    if (active.waitingForFrame || active.waitTimer > 0.0f) {
        return "waiting";
    }
    return "running";
}

std::string ScriptManager::describeActiveScript(size_t index, const ActiveScript& active) {
    std::ostringstream oss;
    oss << "[" << index << "] ";
    if (active.isInlineScript) {
        oss << "inline quest=0x" << hexU32(active.inlineKey.questFormID)
            << " stage=" << active.inlineKey.stageIndex
            << " block=" << active.inlineKey.scriptIndex;
    } else {
        oss << "0x" << hexU32(active.scriptFormID);
        const ScriptData* script = active.context.getScript();
        if (script && !script->editorID.empty()) {
            oss << " \"" << script->editorID << "\"";
        }
    }
    oss << " self=0x" << hexU32(active.selfRefFormID)
        << " pc=0x" << hexOffset(active.context.getPC())
        << " state=" << describeScriptState(active);
    return oss.str();
}

std::string ScriptManager::formatDisasmWindow(const ScriptData& script, uint32_t pc,
                                              int before, int after) const {
    const std::vector<uint32_t> offsets =
        ScriptDisasm::instructionOffsets(script.bytecode.data(), script.bytecode.size());
    if (offsets.empty()) {
        return "(no bytecode)\n";
    }

    // The window anchors on the last instruction boundary at or before the PC.
    size_t pos = 0;
    while (pos + 1 < offsets.size() && offsets[pos + 1] <= pc) {
        ++pos;
    }
    const bool onBoundary = offsets[pos] == pc;

    const size_t start = pos > static_cast<size_t>(before) ? pos - before : 0;
    const size_t end = std::min(offsets.size(), pos + static_cast<size_t>(after) + 1);

    std::ostringstream oss;
    oss << ScriptDisasm::disassembleRange(
        script.bytecode.data(), script.bytecode.size(),
        offsets[start], static_cast<int>(end - start),
        onBoundary ? pc : ScriptDisasm::NO_MARKER);
    if (!onBoundary) {
        oss << "(PC 0x" << hexOffset(pc) << " is not on an instruction boundary)\n";
    }
    return oss.str();
}

std::string ScriptManager::formatFault(const ActiveScript& active) const {
    const ScriptData* script = active.context.getScript();
    std::ostringstream oss;
    oss << "Script fault";
    if (active.isInlineScript) {
        oss << " (inline quest=0x" << hexU32(active.inlineKey.questFormID)
            << " stage=" << active.inlineKey.stageIndex << ")";
    } else {
        oss << " 0x" << hexU32(active.scriptFormID);
        if (script && !script->editorID.empty()) {
            oss << " \"" << script->editorID << "\"";
        }
    }
    oss << " on self 0x" << hexU32(active.selfRefFormID)
        << " at PC=0x" << hexOffset(vm_.getLastErrorPC())
        << ": " << vm_.getLastError() << "\n";
    if (script && !script->bytecode.empty()) {
        oss << formatDisasmWindow(*script, vm_.getLastErrorPC(), 6, 6);
    }
    return oss.str();
}

std::string ScriptManager::debugListScripts() const {
    std::ostringstream oss;
    oss << "Active scripts: " << activeScripts_.size()
        << " / loaded: " << scripts_.size()
        << " / breakpoints: " << vm_.getBreakpoints().size() << "\n";
    if (activeScripts_.empty()) {
        oss << "(none)";
        return oss.str();
    }
    for (size_t i = 0; i < activeScripts_.size(); ++i) {
        oss << describeActiveScript(i, activeScripts_[i]) << "\n";
    }
    return oss.str();
}

std::string ScriptManager::debugScriptInfo(int index) {
    const int i = resolveDebugIndex(index);
    if (i < 0) {
        return i == -2
            ? "Script index " + std::to_string(index) + " out of range (" +
                  std::to_string(activeScripts_.size()) + " active)"
            : "No active scripts";
    }
    debugSelectedScript_ = i;

    const ActiveScript& active = activeScripts_[i];
    const ScriptData* script = active.context.getScript();

    std::ostringstream oss;
    oss << "Script " << describeActiveScript(static_cast<size_t>(i), active) << "\n";
    if (!script) {
        oss << "(script data unavailable)";
        return oss.str();
    }
    oss << "type=" << static_cast<int>(script->scriptType)
        << " bytecode=" << script->bytecode.size() << "B"
        << " locals=" << active.context.getLocalCount()
        << " stack=" << active.context.stackSize()
        << " insns=" << active.context.getInstructionCount()
        << " target=0x" << hexU32(active.context.getTargetRef()) << "\n";
    oss << formatDisasmWindow(*script, active.context.getPC(), 4, 10);
    return oss.str();
}

std::string ScriptManager::debugDisasm(int index, uint32_t startOffset, int count) {
    const int i = resolveDebugIndex(index);
    if (i < 0) {
        return i == -2
            ? "Script index " + std::to_string(index) + " out of range (" +
                  std::to_string(activeScripts_.size()) + " active)"
            : "No active scripts";
    }
    debugSelectedScript_ = i;

    const ActiveScript& active = activeScripts_[i];
    const ScriptData* script = active.context.getScript();
    if (!script || script->bytecode.empty()) {
        return "Script has no bytecode";
    }

    const uint32_t pc = active.context.getPC();
    const uint32_t start = startOffset == ScriptDisasm::NO_MARKER ? pc : startOffset;
    if (start >= script->bytecode.size()) {
        return "Start offset 0x" + hexOffset(start) + " out of range (bytecode " +
               std::to_string(script->bytecode.size()) + "B)";
    }

    const int limit = count > 0 ? std::min(count, 512) : 32;

    std::ostringstream oss;
    oss << "Disassembly of " << describeActiveScript(static_cast<size_t>(i), active) << "\n";
    oss << ScriptDisasm::disassembleRange(script->bytecode.data(), script->bytecode.size(),
                                          start, limit, pc);
    return oss.str();
}

std::string ScriptManager::applyDebugStep(ActiveScript& active, size_t index, bool resume) {
    const ScriptData* script = active.context.getScript();
    const uint32_t pcBefore = active.context.getPC();
    const std::string executed =
        script && pcBefore < script->bytecode.size()
            ? ScriptDisasm::disassembleInstruction(script->bytecode.data(),
                                                   script->bytecode.size(), pcBefore)
            : "<out of bounds>";

    const VMResult result = vm_.step(active.context);

    std::ostringstream oss;
    oss << (resume ? "Resumed" : "Stepped") << " [" << index << "] 0x"
        << hexOffset(pcBefore) << ": " << executed;

    if (result == VMResult::Error) {
        oss << "\n" << formatFault(active);
        releaseInlineScript(active);
        activeScripts_.erase(activeScripts_.begin() + static_cast<ptrdiff_t>(index));
        clampDebugSelection();
        return oss.str();
    }

    if (!active.context.isRunning() || result == VMResult::NotRunning) {
        oss << " -> script completed";
        releaseInlineScript(active);
        activeScripts_.erase(activeScripts_.begin() + static_cast<ptrdiff_t>(index));
        clampDebugSelection();
        return oss.str();
    }

    // Stepping keeps the script paused at the new PC; resuming lets the next
    // frame continue from it.
    active.paused = !resume;
    oss << " -> PC=0x" << hexOffset(active.context.getPC())
        << (resume ? " (running)" : " (paused)");
    return oss.str();
}

std::string ScriptManager::debugStepScript(int index) {
    const int i = resolveDebugIndex(index);
    if (i < 0) {
        return i == -2
            ? "Script index " + std::to_string(index) + " out of range (" +
                  std::to_string(activeScripts_.size()) + " active)"
            : "No active scripts";
    }
    debugSelectedScript_ = i;
    return applyDebugStep(activeScripts_[i], static_cast<size_t>(i), false);
}

std::string ScriptManager::debugResumeScript(int index) {
    const int i = resolveDebugIndex(index);
    if (i < 0) {
        return i == -2
            ? "Script index " + std::to_string(index) + " out of range (" +
                  std::to_string(activeScripts_.size()) + " active)"
            : "No active scripts";
    }
    debugSelectedScript_ = i;

    ActiveScript& active = activeScripts_[i];
    if (!active.paused) {
        return "Script [" + std::to_string(i) + "] is not paused (state=" +
               describeScriptState(active) + ")";
    }

    // Stepping past the breakpoint once is what lets execution continue; a
    // plain unpause would stop on the same breakpoint again every frame.
    return applyDebugStep(active, static_cast<size_t>(i), true);
}

std::string ScriptManager::debugToggleBreakpoint(int index, uint32_t offset) {
    const int i = resolveDebugIndex(index);
    if (i < 0) {
        return i == -2
            ? "Script index " + std::to_string(index) + " out of range (" +
                  std::to_string(activeScripts_.size()) + " active)"
            : "No active scripts";
    }
    debugSelectedScript_ = i;

    const ActiveScript& active = activeScripts_[i];
    const ScriptData* script = active.context.getScript();
    if (!script || script->bytecode.empty()) {
        return "Script has no bytecode";
    }
    if (offset >= script->bytecode.size()) {
        return "Offset 0x" + hexOffset(offset) + " out of range (bytecode " +
               std::to_string(script->bytecode.size()) + "B)";
    }

    bool onBoundary = false;
    for (const uint32_t candidate :
         ScriptDisasm::instructionOffsets(script->bytecode.data(), script->bytecode.size())) {
        if (candidate == offset) {
            onBoundary = true;
            break;
        }
    }

    const bool enabled = !vm_.hasBreakpoint(script, offset);
    vm_.setBreakpoint(script, offset, enabled);

    std::ostringstream oss;
    oss << (enabled ? "Breakpoint set" : "Breakpoint cleared") << " at 0x"
        << hexOffset(offset) << " " << describeActiveScript(static_cast<size_t>(i), active);
    if (!onBoundary) {
        oss << "\nwarning: 0x" << hexOffset(offset)
            << " is not an instruction boundary; the breakpoint will not fire";
    }
    return oss.str();
}

std::string ScriptManager::debugListBreakpoints() const {
    const std::vector<ScriptBreakpoint>& breakpoints = vm_.getBreakpoints();
    std::ostringstream oss;
    oss << "Breakpoints: " << breakpoints.size() << "\n";
    for (const ScriptBreakpoint& bp : breakpoints) {
        oss << "- 0x" << hexOffset(bp.offset);

        bool described = false;
        for (size_t i = 0; i < activeScripts_.size(); ++i) {
            if (activeScripts_[i].context.getScript() == bp.script) {
                oss << " " << describeActiveScript(i, activeScripts_[i]);
                described = true;
                break;
            }
        }
        if (described) {
            oss << "\n";
            continue;
        }

        bool loaded = false;
        for (const auto& entry : scripts_) {
            if (&entry.second == bp.script) {
                oss << " 0x" << hexU32(entry.first);
                if (!entry.second.editorID.empty()) {
                    oss << " \"" << entry.second.editorID << "\"";
                }
                oss << " (not running)\n";
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            oss << " (stale)\n";
        }
    }
    return oss.str();
}

std::string ScriptManager::debugClearBreakpoints() {
    const size_t count = vm_.getBreakpoints().size();
    vm_.clearBreakpoints();
    return "Cleared " + std::to_string(count) + " breakpoint(s)";
}

std::string ScriptManager::debugScriptVariables(int index) {
    const int i = resolveDebugIndex(index);
    if (i < 0) {
        return i == -2
            ? "Script index " + std::to_string(index) + " out of range (" +
                  std::to_string(activeScripts_.size()) + " active)"
            : "No active scripts";
    }
    debugSelectedScript_ = i;

    const ActiveScript& active = activeScripts_[i];
    const ScriptData* script = active.context.getScript();

    std::ostringstream oss;
    oss << "Variables of " << describeActiveScript(static_cast<size_t>(i), active) << "\n";
    oss << "locals (" << active.context.getLocalCount() << "):\n";
    for (size_t slot = 0; slot < active.context.getLocalCount(); ++slot) {
        std::string name;
        if (script) {
            for (const ScriptVariable& variable : script->variables) {
                if (variable.index == slot) {
                    name = variable.name;
                    break;
                }
            }
        }
        oss << "  [" << slot << "] " << (name.empty() ? "(unnamed)" : name)
            << " = " << ScriptDisasm::formatValue(active.context.getLocal(static_cast<uint16_t>(slot)))
            << "\n";
    }
    oss << "stack (" << active.context.stackSize() << ")"
        << " top=" << ScriptDisasm::formatValue(active.context.peekStack());
    return oss.str();
}

std::string ScriptManager::debugListGlobals() const {
    std::vector<uint32_t> keys;
    keys.reserve(globalVariables_.size());
    for (const auto& entry : globalVariables_) {
        keys.push_back(entry.first);
    }
    std::sort(keys.begin(), keys.end());

    std::ostringstream oss;
    oss << "Globals: " << keys.size() << "\n";
    for (const uint32_t key : keys) {
        oss << "  0x" << hexU32(key) << " = "
            << ScriptDisasm::formatValue(globalVariables_.at(key)) << "\n";
    }
    if (keys.empty()) {
        oss << "(none)";
    }
    return oss.str();
}

std::string ScriptManager::debugGetGlobal(uint32_t formID) const {
    auto it = globalVariables_.find(formID);
    if (it == globalVariables_.end()) {
        return "Global 0x" + hexU32(formID) + " not set (reads as 0)";
    }
    return "0x" + hexU32(formID) + " = " + ScriptDisasm::formatValue(it->second);
}

std::string ScriptManager::debugSetGlobal(uint32_t formID, const std::string& valueText) {
    if (valueText.empty()) {
        return "Usage: setglobal <formID> <value>";
    }
    if (globalVariables_.size() >= static_cast<size_t>(limits::MAX_GLOBAL_VARIABLES) &&
        globalVariables_.find(formID) == globalVariables_.end()) {
        return "Global variable limit reached (" +
               std::to_string(limits::MAX_GLOBAL_VARIABLES) + ")";
    }

    ScriptValue value;
    try {
        if (valueText.find('.') != std::string::npos) {
            value = ScriptValue::makeFloat(std::stof(valueText));
        } else {
            const int base = (valueText.rfind("0x", 0) == 0 || valueText.rfind("0X", 0) == 0)
                                 ? 16
                                 : 10;
            value = ScriptValue::makeInt(static_cast<int32_t>(
                std::stoll(valueText, nullptr, base)));
        }
    } catch (...) {
        return "Invalid value '" + valueText + "' (use an integer, 0x-hex or a decimal float)";
    }

    const std::string previous = debugGetGlobal(formID);
    globalVariables_[formID] = value;
    return "0x" + hexU32(formID) + " = " + ScriptDisasm::formatValue(value) +
           " (was: " + previous + ")";
}

std::string ScriptManager::debugRunScript(uint32_t formID, uint32_t selfRefFormID) {
    const ScriptData* script = getScript(formID);
    if (!script) {
        return "Script 0x" + hexU32(formID) + " not loaded";
    }
    if (isScriptRunning(formID, selfRefFormID)) {
        return "Script 0x" + hexU32(formID) + " is already running on self 0x" +
               hexU32(selfRefFormID);
    }

    const int started = startScript(formID, selfRefFormID);
    if (started < 0) {
        return "Failed to start script 0x" + hexU32(formID);
    }
    debugSelectedScript_ = started;
    return "Started [" + std::to_string(started) + "] 0x" + hexU32(formID) +
           (script->editorID.empty() ? "" : " \"" + script->editorID + "\"") +
           " on self 0x" + hexU32(selfRefFormID);
}

} // namespace script
} // namespace oblivion
