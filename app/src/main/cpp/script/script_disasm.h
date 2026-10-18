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

    // Offset of the instruction that follows the one at offset. Returns size
    // when the offset is at or past the end, and never moves backwards, so a
    // caller that walks the stream always terminates.
    static uint32_t nextInstructionOffset(const uint8_t* bytecode, size_t size, uint32_t offset);

    // Start offsets of every instruction in the bytecode, capped with a safety
    // limit for malformed streams.
    static std::vector<uint32_t> instructionOffsets(const uint8_t* bytecode, size_t size);

    // Disassemble up to maxInstructions starting at startOffset. The line at
    // markerOffset is prefixed with ">> " so a debugger view can flag the PC.
    // Pass SCRIPT_DISASM_NO_MARKER to disable the marker.
    static constexpr uint32_t NO_MARKER = 0xFFFFFFFFu;
    static std::string disassembleRange(const uint8_t* bytecode, size_t size,
                                        uint32_t startOffset, int maxInstructions,
                                        uint32_t markerOffset);

    // Format a value the way the disassembler's variable listing does.
    static std::string formatValue(const ScriptValue& value);

private:
    // Format argument data as hex string
    static std::string formatArgs(const uint8_t* data, uint16_t length);
};

} // namespace script
} // namespace oblivion
