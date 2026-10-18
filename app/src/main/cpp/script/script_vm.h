#pragma once

#include "script_context.h"
#include "script_functions.h"
#include <memory>
#include <string>
#include <android/log.h>

// ============================================================================
// Oblivion Script VM - Bytecode Execution Engine
// Executes compiled Oblivion script bytecode instruction by instruction
// ============================================================================

#define SCRIPT_LOG_TAG "ScriptVM"
#ifdef ENABLE_DEBUG_LOGS
#define SCRIPT_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, SCRIPT_LOG_TAG, __VA_ARGS__)
#else
#define SCRIPT_LOGD(...) do {} while(0)
#endif
#define SCRIPT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, SCRIPT_LOG_TAG, __VA_ARGS__)
#define SCRIPT_LOGW(...) __android_log_print(ANDROID_LOG_WARN, SCRIPT_LOG_TAG, __VA_ARGS__)
#define SCRIPT_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SCRIPT_LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace script {

// ============================================================================
// VM execution result
// ============================================================================
enum class VMResult {
    Success,        // Script completed normally (STOP opcode)
    FrameBudget,    // Hit per-frame instruction limit (will resume next frame)
    Breakpoint,     // Stopped on a breakpoint; the instruction did not run
    Error,          // Execution error
    NotRunning,     // Script not running
};

// One debugger breakpoint. Breakpoints are keyed by the script data the
// context executes, so a breakpoint follows the script, not a running
// instance, and survives pauses across frames.
struct ScriptBreakpoint {
    const ScriptData* script = nullptr;
    uint32_t offset = 0;    // Byte offset into the script's bytecode
};

// ============================================================================
// ScriptVM - executes bytecode
// ============================================================================
class ScriptVM {
public:
    ScriptVM();
    ~ScriptVM() = default;

    // Set the function handler registry
    void setFunctions(ScriptFunctions* functions) { functions_ = functions; }

    // Execute bytecode in the given context
    // Returns VMResult indicating outcome
    VMResult execute(ExecutionContext& ctx, int maxInstructions = limits::MAX_INSTRUCTIONS_PER_FRAME);

    // Execute a single instruction (for stepping/debugging)
    VMResult step(ExecutionContext& ctx);

    // Get last error message
    const std::string& getLastError() const { return lastError_; }

    // Byte offset of the instruction the last error occurred on
    uint32_t getLastErrorPC() const { return lastErrorPC_; }

    // Breakpoints (debugger). The per-instruction check is skipped entirely
    // while the list is empty, so normal execution pays nothing for this.
    void setBreakpoint(const ScriptData* script, uint32_t offset, bool enabled);
    bool hasBreakpoint(const ScriptData* script, uint32_t offset) const;
    void removeBreakpointsFor(const ScriptData* script);
    void clearBreakpoints() { breakpoints_.clear(); }
    const std::vector<ScriptBreakpoint>& getBreakpoints() const { return breakpoints_; }

private:
    ScriptFunctions* functions_ = nullptr;
    std::string lastError_;
    uint32_t lastErrorPC_ = 0;
    std::vector<ScriptBreakpoint> breakpoints_;

    // True when the context's PC matches a breakpoint
    bool isBreakpointHit(const ExecutionContext& ctx) const;

    // Instruction decoding
    bool decodeInstruction(const ExecutionContext& ctx, Instruction& inst);

    // Opcode handlers
    VMResult executeArithmetic(ExecutionContext& ctx, Opcode op);
    VMResult executeComparison(ExecutionContext& ctx, Opcode op);
    VMResult executeLogical(ExecutionContext& ctx, Opcode op);
    VMResult executeControlFlow(ExecutionContext& ctx, Opcode op, const Instruction& inst);
    VMResult executeStackOp(ExecutionContext& ctx, Opcode op, const Instruction& inst);
    VMResult executeVariableOp(ExecutionContext& ctx, Opcode op, const Instruction& inst);
    VMResult executeStringOp(ExecutionContext& ctx, Opcode op);
    VMResult executeRefOp(ExecutionContext& ctx, Opcode op);
    VMResult executeCall(ExecutionContext& ctx, const Instruction& inst);

    // Helper: read little-endian values from bytecode
    template<typename T>
    T readArg(const uint8_t* data) const {
        T value;
        std::memcpy(&value, data, sizeof(T));
        return value;
    }

    void setError(const std::string& msg) {
        lastError_ = msg;
        SCRIPT_LOGE("%s", msg.c_str());
    }
};

} // namespace script
} // namespace oblivion
