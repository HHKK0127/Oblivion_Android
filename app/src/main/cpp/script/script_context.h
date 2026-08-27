#pragma once

#include "script_opcodes.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <memory>

// ============================================================================
// Oblivion Script VM - Execution Context
// Manages RPN stack, local variables, reference table, and program counter
// ============================================================================

namespace oblivion {
namespace script {

// ============================================================================
// Script variable definition (from SLSD/SCVR subrecords)
// ============================================================================
struct ScriptVariable {
    uint32_t index = 0;         // Variable index
    std::string name;           // Variable name (from SCVR)
    ScriptValue::Type type = ScriptValue::Type::Integer;
    ScriptValue defaultValue;   // Initial value
};

// ============================================================================
// Script data (parsed from SCPT record)
// ============================================================================
struct ScriptData {
    uint32_t formID = 0;
    std::string editorID;           // EDID
    ScriptType scriptType = ScriptType::Object;
    uint32_t refCount = 0;          // Number of references
    uint32_t compiledLength = 0;    // Bytecode length
    uint32_t lastVarIndex = 0;
    uint32_t varCount = 0;
    std::vector<uint8_t> bytecode;  // SCDA - compiled bytecode
    std::string source;             // SCTX - source text (debug)
    std::vector<ScriptVariable> variables;  // SLSD/SCVR pairs
    std::vector<uint32_t> references;       // SCRO - referenced FormIDs
};

// ============================================================================
// Execution Context - per-script-instance state
// ============================================================================
class ExecutionContext {
public:
    ExecutionContext() = default;
    ~ExecutionContext() = default;

    // Initialize for a specific script
    void init(const ScriptData* script);

    // Stack operations
    bool pushStack(const ScriptValue& value);
    ScriptValue popStack();
    ScriptValue peekStack() const;
    bool isStackEmpty() const { return stack_.empty(); }
    size_t stackSize() const { return stack_.size(); }

    // Local variable access
    void setLocal(uint16_t index, const ScriptValue& value);
    ScriptValue getLocal(uint16_t index) const;

    // Program counter
    uint32_t getPC() const { return pc_; }
    void setPC(uint32_t pc) { pc_ = pc; }
    void advancePC(uint32_t offset) { pc_ += offset; }

    // Reference table
    void addReference(uint32_t index, uint32_t formID);
    uint32_t getReference(uint32_t index) const;
    uint32_t getSelfRef() const { return selfRef_; }
    void setSelfRef(uint32_t formID) { selfRef_ = formID; }
    uint32_t getTargetRef() const { return targetRef_; }
    void setTargetRef(uint32_t formID) { targetRef_ = formID; }

    // State
    bool isRunning() const { return running_; }
    void stop() { running_ = false; }
    void reset();

    // Instruction counter (for frame budget)
    int getInstructionCount() const { return instructionCount_; }
    void incrementInstructionCount() { ++instructionCount_; }
    void resetInstructionCount() { instructionCount_ = 0; }

    // Script data accessor
    const ScriptData* getScript() const { return script_; }

    // Bytecode accessor
    const uint8_t* getBytecode() const { return bytecode_; }
    size_t getBytecodeSize() const { return bytecodeSize_; }

private:
    const ScriptData* script_ = nullptr;
    const uint8_t* bytecode_ = nullptr;
    size_t bytecodeSize_ = 0;

    // RPN stack
    std::vector<ScriptValue> stack_;

    // Local variables
    std::vector<ScriptValue> locals_;

    // Reference table (index -> FormID)
    std::unordered_map<uint32_t, uint32_t> references_;
    uint32_t selfRef_ = 0;
    uint32_t targetRef_ = 0;

    // Program counter
    uint32_t pc_ = 0;

    // State
    bool running_ = false;
    int instructionCount_ = 0;
};

} // namespace script
} // namespace oblivion
