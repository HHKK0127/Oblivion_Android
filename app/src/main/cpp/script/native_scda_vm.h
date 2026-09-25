#pragma once

#include "native_scda_decoder.h"
#include "script_opcodes.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// Native SCDA virtual machine
//
// Executes the instruction stream produced by NativeScdaDecoder. This is a
// separate execution space from ScriptVM: the synthetic VM runs the
// Opcode/FunctionID encoding, while this one runs the compiler's own encoding.
// The two must never share an opcode table.
//
// Execution model
// ---------------
// The stream is a flat instruction array. Control flow is a depth stack, not a
// forward scan: If pushes a frame, ElseIf/Else consult the innermost frame, and
// EndIf pops it. A stray EndIf, Else or ElseIf at depth zero is a no-op, because
// the retail corpus contains 43 scripts that Bethesda shipped with an unbalanced
// block (an endif with no if, or an extra endif before an else). Treating those
// as corruption would reject real content.
//
// Begin selects the block to run by block type. The block ends at
// begin.offset + 8 + begin.bodyLength, which is the End instruction, so End and
// Return both stop the run.
//
// Expressions are reverse Polish. The operator set was measured over the whole
// vanilla corpus and is closed: == != < <= > >= + - * / && || and the unary ~.
// There is no modulo and no unary minus; a leading '-' is always binary
// subtraction. Numeric literals arrive as integer text or as decimal text such
// as .001, .01, .1, .5, 0.1, 0.5 and 1.5.
// ============================================================================

namespace oblivion {
namespace script {

// Outcome of a run.
enum class NativeVmResult {
    Success,        // The block ran to its End or a Return
    FrameBudget,    // The instruction budget ran out; the run can be resumed
    Error,          // Execution error, described by getLastError()
    NotRunning,     // No block matched the requested block type
};

// One If/ElseIf/Else chain frame.
struct NativeIfFrame {
    size_t ifIndex = 0;      // Instruction index of the If that opened the frame
    bool branchTaken = false; // A branch of this chain has already run
};

// Local variable storage. The native encoding indexes locals by slot, and the
// slot space is shared by the f/s/l type chars, so one vector serves all three.
class NativeVariableStore {
public:
    void clear() { values_.clear(); }

    void set(uint16_t index, const ScriptValue& value) {
        if (index >= values_.size()) {
            values_.resize(static_cast<size_t>(index) + 1);
        }
        values_[index] = value;
    }

    ScriptValue get(uint16_t index) const {
        if (index >= values_.size()) {
            return ScriptValue::makeInt(0);
        }
        return values_[index];
    }

    size_t size() const { return values_.size(); }

private:
    std::vector<ScriptValue> values_;
};

// A command handler. The reference is the selector that preceded the command,
// or 0 when the command had none. The tokens are the decoded argument list.
struct NativeCommandContext {
    uint32_t selfRef = 0;        // FormID the script is attached to
    uint32_t targetRef = 0;      // FormID passed as the call target
    uint16_t referenceIndex = 0; // Raw selector slot, 0 when absent
    uint32_t referenceFormId = 0; // Resolved selector FormID, 0 when absent
    uint16_t opcode = 0;
    uint32_t offset = 0;         // Byte offset of the command in the bytecode
    // Local variable storage of the running VM. Argument tokens of the f/s/l
    // type chars name a slot rather than a value, so a handler that needs the
    // value reads it here. Null when the VM has no store attached.
    const NativeVariableStore* variables = nullptr;
};

using NativeCommandHandler = std::function<bool(NativeCommandContext& context,
                                                const std::vector<NativeToken>& args,
                                                ScriptValue& returnValue,
                                                std::string& error)>;

// A handler for a function used inside an expression, such as GetStage or
// IsActionRef. Its arguments arrive already evaluated, because an expression
// operand is a value rather than a token list.
using NativeExpressionHandler =
    std::function<bool(NativeCommandContext& context,
                       const std::vector<ScriptValue>& args,
                       ScriptValue& returnValue,
                       std::string& error)>;

class NativeScdaVm {
public:
    NativeScdaVm();
    ~NativeScdaVm() = default;

    // Registers a handler for an opcode. Registering the same opcode twice
    // replaces the previous handler.
    void registerCommand(uint16_t opcode, NativeCommandHandler handler);

    // Registers a handler for a function used inside an expression. The same
    // opcode number space is shared with the commands, so a function that is
    // also a command can be registered in both tables.
    void registerExpressionFunction(uint16_t opcode, NativeExpressionHandler handler);

    // Registers a handler under every spelling the opcode accepts, so a caller
    // can look a command up by name as well as by number.
    void registerCommandByName(uint16_t opcode, NativeCommandHandler handler);

    bool hasCommand(uint16_t opcode) const;
    bool hasExpressionFunction(uint16_t opcode) const;

    // Runs the block of the given type. blockType 0 is the gamemode block, which
    // is what a quest or object script runs on load.
    NativeVmResult run(const NativeDecodeResult& program,
                       uint16_t blockType,
                       uint32_t selfRef,
                       uint32_t targetRef = 0,
                       int maxInstructions = 100000);

    // Resumes a run that stopped on FrameBudget.
    NativeVmResult resume(int maxInstructions = 100000);

    // Evaluates a decoded expression against the current variable store. Exposed
    // so tests and callers can evaluate a condition without running a block.
    bool evaluateExpression(const std::vector<NativeToken>& tokens,
                            ScriptValue& out,
                            std::string& error);

    // Local variable storage, shared with the caller so a script's state
    // survives across runs.
    NativeVariableStore& variables() { return variables_; }
    const NativeVariableStore& variables() const { return variables_; }

    // Reference table used to resolve selector slots. Slots up to the SCRO count
    // resolve to SCRO entries; the remainder resolve to local refs.
    void setReferences(const std::vector<uint32_t>& scroRefs, uint16_t localRefCount);

    // Global variables, keyed by FormID, as the 'G' type char addresses them.
    void setGlobal(uint32_t formId, const ScriptValue& value);
    ScriptValue getGlobal(uint32_t formId) const;

    // Resolves a selector slot through the reference table. Returns false and
    // fills error when the slot is out of range.
    bool resolveReference(uint16_t slot, NativeReferenceSlot& out, std::string& error) const;

    const std::string& getLastError() const { return lastError_; }
    uint32_t getLastErrorOffset() const { return lastErrorOffset_; }
    uint16_t getLastErrorOpcode() const { return lastErrorOpcode_; }

    // Number of instructions executed by the most recent run.
    int getExecutedInstructionCount() const { return executedInstructions_; }

    // Number of commands that had no registered handler during the last run.
    int getUnhandledCommandCount() const { return unhandledCommands_; }

private:
    // Program state, kept so a budgeted run can resume.
    const NativeDecodeResult* program_ = nullptr;
    size_t pc_ = 0;
    size_t blockEnd_ = 0;
    uint16_t blockType_ = 0;
    uint32_t selfRef_ = 0;
    uint32_t targetRef_ = 0;
    bool running_ = false;
    int executedInstructions_ = 0;
    int unhandledCommands_ = 0;

    std::vector<NativeIfFrame> ifStack_;
    std::vector<ScriptValue> stack_;

    NativeVariableStore variables_;
    std::unordered_map<uint32_t, ScriptValue> globals_;
    std::vector<uint32_t> scroRefs_;
    uint16_t localRefCount_ = 0;
    // A 0x001C marker names the reference the next command acts on, so the
    // selector is held here until that command consumes it.
    uint16_t pendingReferenceIndex_ = 0;

    std::unordered_map<uint16_t, NativeCommandHandler> commands_;
    std::unordered_map<uint16_t, NativeExpressionHandler> expressionFunctions_;

    std::string lastError_;
    uint32_t lastErrorOffset_ = 0;
    uint16_t lastErrorOpcode_ = 0;

    void setError(const std::string& message, uint32_t offset, uint16_t opcode);

    // Instruction index of the matching EndIf for the If at ifIndex, or the
    // program size when the stream is unbalanced.
    size_t findEndIf(size_t ifIndex) const;

    // Instruction index of the next ElseIf/Else/EndIf at the same depth, or the
    // program size when there is none.
    size_t findNextBranch(size_t fromIndex) const;

    bool pushValue(const ScriptValue& value, std::string& error);
    bool popValue(ScriptValue& out, std::string& error);

    bool evaluateToken(const NativeToken& token, ScriptValue& out, std::string& error);
    bool applyOperator(const std::string& op, std::string& error);

    bool executeInstruction(const NativeInstruction& instruction, bool& stopped,
                            std::string& error);
    bool executeSet(const NativeInstruction& instruction, std::string& error);
    bool executeCommand(const NativeInstruction& instruction, std::string& error);
};

} // namespace script
} // namespace oblivion
