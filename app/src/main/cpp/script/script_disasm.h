#pragma once

#include "script_opcodes.h"
#include "script_context.h"
#include <string>
#include <vector>

// ============================================================================
// Oblivion Script VM - Disassembler (Debug)
// Converts bytecode to human-readable assembly for debugging
// ============================================================================

namespace oblivion {
namespace script {

class ScriptDisasm {
public:
    ScriptDisasm() = default;
    ~ScriptDisasm() = default;

    // Disassemble a script's bytecode into readable text
    static std::string disassemble(const ScriptData& script);

    // Disassemble a single instruction at a given offset
    static std::string disassembleInstruction(const uint8_t* bytecode, size_t size, uint32_t offset);

    // Get opcode name
    static const char* getOpcodeName(Opcode op);

    // Get function name from ID
    static const char* getFunctionName(uint16_t funcID);

private:
    // Format argument data as hex string
    static std::string formatArgs(const uint8_t* data, uint16_t length);
};

} // namespace script
} // namespace oblivion
