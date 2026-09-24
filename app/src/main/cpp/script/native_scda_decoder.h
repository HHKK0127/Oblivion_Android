#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// Native SCDA decoder
//
// Decodes the compiled bytecode stored in SCDA subrecords. This is a separate
// instruction space from the synthetic Opcode/FunctionID enums: native opcodes
// are the values the Oblivion compiler emitted, and their payloads are not the
// synthetic stack encoding. Never remap native values onto Opcode/FunctionID.
//
// Grammar confirmed against the vanilla Oblivion3.esm census:
//   [u16 opcode][u16 argLength][argLength bytes]
//   opcode 0x001C is always 4 bytes; its second u16 is a call reference index.
//
// ---------------------------------------------------------------------------
// SCPT grammar memo (verified against the full vanilla census; keep for later
// workstreams so the findings do not have to be re-derived)
//
// Structural opcodes and their payloads:
//   0x0010 Begin   [u16 blockType][u16 bodyLength][u32 0/1] (+ optional ref arg)
//   0x0011 End     no payload
//   0x0015 Set     [u16 argc][typed tokens]
//   0x0016 If      [u16 meta][u16 expressionLength][expression bytes]
//   0x0017 Else    [u16 meta] only, exactly 2 bytes
//   0x0018 ElseIf  same shape as 0x0016
//   0x0019 EndIf   no payload
//   0x001E Return  no payload
//   0x001C marker  exactly 4 bytes; the second u16 is a reference index
//   0x001D prologue: SCPT only. QUST/INFO inline scripts never carry it, so a
//          decoder must not require it.
//
// Expressions are reverse Polish with ASCII operators kept as text, and
// operands encoded as typed tokens (see NativeTokenKind). Function calls inside
// expressions use the 'X' token, whose u16 id shares the opcode number space.
//
// The full census walks every SCDA block with zero misalignment, so the
// framing above is closed. SCDA is the only executable payload: a block with an
// SCTX source but no SCDA simply executes nothing and is not an error.
// ---------------------------------------------------------------------------

namespace oblivion {
namespace script {

// Structural opcodes. These are the only opcodes whose payload is interpreted
// by the decoder itself rather than treated as an opaque argument blob.
enum class NativeStructuralOpcode : uint16_t {
    Begin  = 0x0010,  // [u16 blockType][u16 bodyLength][u32 0/1] + optional arg
    End    = 0x0011,  // no payload
    Set    = 0x0015,  // [u16 argc][typed tokens]
    If     = 0x0016,  // [u16 meta][u16 expressionLength][expression bytes]
    Else   = 0x0017,  // [u16 meta]
    ElseIf = 0x0018,  // [u16 meta][u16 expressionLength][expression bytes]
    EndIf  = 0x0019,  // no payload
    Return = 0x001E,  // no payload
};

// Call reference selector. Its second u16 is a reference index, not a length:
// it selects the reference used by the command that follows, which is how the
// compiler encodes `Ref.command`. The 4 bytes must always be consumed and the
// index applied, otherwise every bare `Ref.command` resolves to the wrong ref.
//
// The operand is a 1-based slot in the script's reference table. Two properties
// were verified against the full vanilla census:
//   1. Slots are handed out densely from 1 in first-appearance order across the
//      bytecode; re-using an existing slot does not consume a new one. All 1,173
//      records that carry a selector produce exactly 1..K with no gaps.
//   2. The table bound is `SCRO count + local ref declaration count`. Both the
//      selector operand and the `r` variable token index stay within it, with
//      zero violations over 1,654 records carrying `r` tokens and 1,173 carrying
//      selectors, so both index the same table.
//
// What is NOT settled is the relative order of the two halves. The vanilla
// corpus contains examples of both readings, so resolution must stay
// conservative: treat slots up to the SCRO count as SCRO entries and any
// remaining slot as a local ref, and report an out-of-range slot as an explicit
// error instead of inventing a reference.
constexpr uint16_t NATIVE_SCDA_MARKER_OPCODE = 0x001C;
// Prologue opcode. Present in SCPT records only; QUST/INFO inline scripts do
// not carry it, so a decoder must never require it.
constexpr uint16_t NATIVE_SCDA_PROLOGUE_OPCODE = 0x001D;

// Typed operand token kinds used by the general [u16 argc][tokens] framing.
enum class NativeTokenKind : uint8_t {
    Variable,   // [type char][u16 index]  (f/s/r/l)
    Integer,    // 'n' + u32
    Double,     // 'z' + f64
    Function,   // 'X' + u16 function id + u16 argument byte count
    BareU16,    // any leading byte that is not a type char
    String,     // [u16 byteLength][bytes]
    Unknown,
};

struct NativeToken {
    NativeTokenKind kind = NativeTokenKind::Unknown;
    uint32_t offset = 0;        // Offset of the token inside the payload
    uint32_t length = 0;        // Encoded byte length
    char typeChar = 0;          // Variable type char, when kind == Variable
    uint16_t index = 0;         // Variable index or function id
    int32_t intValue = 0;       // Integer literal or bare u16 value
    double doubleValue = 0.0;   // Double literal
    uint16_t argumentBytes = 0; // Function argument byte count
    std::string text;           // String literal
};

struct NativeInstruction {
    uint16_t opcode = 0;
    uint32_t offset = 0;        // Offset of the opcode inside the bytecode
    uint32_t encodedLength = 0; // Total bytes consumed, including the header
    uint32_t payloadLength = 0; // Bytes after the 4-byte header
    bool isMarker = false;
    bool isPrologue = false;
    bool isStructural = false;
    bool isBare = false;        // Structural opcode with no payload
    uint16_t referenceIndex = 0; // Marker: call reference selector for the next command

    // Structural payloads
    uint16_t blockType = 0;     // Begin
    uint16_t meta = 0;          // If/ElseIf/Else metadata word
    uint32_t bodyLength = 0;    // Begin body length, including its End
    std::vector<uint8_t> expression;  // If/ElseIf expression bytes
    std::vector<NativeToken> tokens;  // Set and general command arguments
    uint16_t declaredArgumentCount = 0;
    bool hasImplicitSelf = false;     // Leading token is the call reference

    // Raw payload for opcodes whose contract is not decoded yet.
    std::vector<uint8_t> payload;
};

struct NativeDecodeResult {
    bool success = false;
    std::string error;
    uint32_t errorOffset = 0;
    uint16_t errorOpcode = 0;
    std::vector<NativeInstruction> instructions;
};

// Decodes a complete SCDA block. The decoder consumes the whole buffer or
// reports the exact offset and opcode where it stopped.
NativeDecodeResult decodeNativeScda(const std::vector<uint8_t>& bytecode);

// Decodes a single instruction starting at offset. Returns false and fills
// error/errorOffset when the instruction cannot be decoded.
bool decodeNativeInstruction(const uint8_t* data, size_t size, uint32_t offset,
                             NativeInstruction& out, std::string& error);

// Human-readable name for a native opcode, or an empty string when unknown.
std::string getNativeOpcodeName(uint16_t opcode);

// Accepted spellings for an opcode. The compiler emits the short spelling more
// often than the long one, so handler registration must accept both.
std::vector<std::string> getNativeOpcodeAliases(uint16_t opcode);

// Kind of reference a selector slot resolves to.
enum class NativeReferenceKind : uint8_t {
    Scro,     // Slot maps to a static SCRO entry
    LocalRef, // Slot maps to a local `ref` variable of the running script
};

struct NativeReferenceSlot {
    NativeReferenceKind kind = NativeReferenceKind::Scro;
    uint32_t formId = 0;     // Valid when kind == Scro
    uint16_t slot = 0;       // Original 1-based slot, always preserved
    uint16_t localOrdinal = 0; // 1-based local ref ordinal when kind == LocalRef
};

// Resolves a 1-based selector slot against a script's SCRO list.
//
// Slots up to scroRefs.size() resolve to SCRO entries. Slots above that resolve
// to a local `ref` variable, reported by ordinal only: the vanilla corpus does
// not settle how a local ref ordinal maps onto the script's variable storage, so
// the caller must not assume it equals the declaration order. A slot of 0, or one
// beyond scroRefs.size() + localRefCount, is an explicit error rather than a
// guessed reference.
bool resolveNativeReferenceSlot(uint16_t slot,
                                const std::vector<uint32_t>& scroRefs,
                                uint16_t localRefCount,
                                NativeReferenceSlot& out,
                                std::string& error);

} // namespace script
} // namespace oblivion
