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
//   0x0015 Set     [1 or 2 typed target tokens][u16 expressionLength][expression]
//   0x0016 If      [u16 meta][u16 expressionLength][expression bytes]
//   0x0017 Else    [u16 meta] only, exactly 2 bytes
//   0x0018 ElseIf  same shape as 0x0016
//   0x0019 EndIf   no payload
//   0x001E Return  no payload
//   0x001C marker  exactly 4 bytes; the second u16 is a reference index
//   0x001D prologue: SCPT only. QUST/INFO inline scripts never carry it, so a
//          decoder must not require it.
//
// Set stores no target count, so the target is one token, or two when the
// statement writes a member of a reference (`Ref.var`). The length equation
// resolves it uniquely: exactly one of the two readings makes
// `targetBytes + 2 + expressionLength` equal the payload length.
//
// Expressions are reverse Polish with ASCII operators kept as text, and
// operands encoded as typed tokens. Dispatch is by leading byte:
//   'f' 's' 'r' 'l' 'G'  [type char][u16 index]            variable, 3 bytes
//   'Z' (0x5A)           [0x5A][u16 index]                 reference literal
//   'n'                  u32 literal, 5 bytes
//   'z'                  f64 literal, 9 bytes
//   'X'                  u16 function id, u16 argument byte count, arguments
//   0x20                 separator, skipped
//   anything else        ASCII text up to the next 0x20 or the end of the
//                        expression (this is where operators live)
// There are no bare u16 operands inside expressions: every non text operand is
// introduced by one of the type chars above. That rule framed every expression
// in the vanilla census without a failure.
//
// 'Z' is a reference operand, not an axis selector. All 20 vanilla occurrences
// sit in Set right hand sides and every one resolved to the expected SCRO entry
// through the 1-based index. See NATIVE_SCDA_REFERENCE_LITERAL_CHAR.
//
// Function calls inside expressions use the 'X' token, whose u16 id shares the
// opcode number space with the command opcodes. Condition-only functions such as
// GetStage and IsActionRef never appear as instructions, only as 'X' tokens, so
// the same handler table must serve both spaces.
//
// Argument lists ([u16 argc][tokens]) use a different dispatcher from
// expressions: 'X' 'Y' 'Z' are one byte axis selectors there, and any other
// leading byte that is not a type char is a bare u16, which is how actor value
// codes such as 0x21 Aggression are encoded.
//
// The full census walks every SCDA block with zero misalignment, so the
// framing above is closed. SCDA is the only executable payload: a block with an
// SCTX source but no SCDA simply executes nothing and is not an error.
// ---------------------------------------------------------------------------

namespace oblivion {
namespace script {

// The 'Z' byte introduces a reference operand. Its u16 payload indexes the
// owning record's SCRO list with a 1-based index, so SCRO[n - 1] is the
// referenced form. All 20 vanilla occurrences were confirmed against SCTX.
constexpr uint8_t NATIVE_SCDA_REFERENCE_LITERAL_CHAR = 0x5A;

// The remote form of Set writes a variable that belongs to another script:
//   [u8 0x72][u16 refSlot][u8 type][u16 remoteVarIndex][u16 elen][expr]
// The local form has no such prefix and places elen right after the target
// token. 0x72 is also the type char of a reference variable, so a local Set
// whose first target is a reference starts with the same byte and both readings
// can satisfy the length equation; the remote reading wins whenever the payload
// starts with 0x72.
constexpr uint8_t NATIVE_SCDA_REMOTE_SET_CHAR = 0x72;

// Structural opcodes. These are the only opcodes whose payload is interpreted
// by the decoder itself rather than treated as an opaque argument blob.
enum class NativeStructuralOpcode : uint16_t {
    Begin  = 0x0010,  // [u16 blockType][u16 bodyLength][u32 0/1] + optional arg
    End    = 0x0011,  // no payload
    Set    = 0x0015,  // [1 or 2 target tokens][u16 expressionLength][expression]
    If     = 0x0016,  // [u16 meta][u16 expressionLength][expression bytes]
    Else   = 0x0017,  // [u16 meta]
    ElseIf = 0x0018,  // [u16 meta][u16 expressionLength][expression bytes]
    EndIf  = 0x0019,  // no payload
    Return = 0x001E,  // no payload
};

// The `begin` block type keyword for a Begin payload's blockType word.
//
// The 25 measured block types map to their source keywords over the 2,351
// records where the keyword and opcode counts agree. The block type lives in
// its own namespace: `0x001E` is `onreset` here while the same number is the
// `return` opcode, so a block type must never be compared against an opcode.
//
// `0x0010` has two spellings in the corpus, `onpackagedone` and
// `onpackageend`, and the compiler emits both for the same block type, so the
// primary name is returned and the alternate is available through
// getNativeBlockTypeAliases.
//
// An unknown block type returns an empty string rather than a guessed keyword.
std::string getNativeBlockTypeName(uint16_t blockType);

// The alternate spellings of a block type, in the order they were measured.
// Only `0x0010` has one; every other block type returns an empty list.
std::vector<std::string> getNativeBlockTypeAliases(uint16_t blockType);

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
//   2. The table bound is the SCRV count plus the SCRO count. Both the selector
//      operand and the `r` variable token index stay within it, with zero
//      violations over 1,654 records carrying `r` tokens and 1,173 carrying
//      selectors, so both index the same table.
//
// The table is the record's ref subrecords in file order, not a fixed
// "SCRO first, then locals" split. See resolveNativeReferenceSlot.
//
// The selector space is NOT the same as the `r`/`Z` slot space. In
// XPXirethard01TrapButton01SCRIPT the file order is SCRV(myParent), SCRO x7,
// SCRV(mySelf), so `myParent` is `r1`; the selector that precedes its Activate
// is also 1, but only because selectors are numbered in first-appearance order
// over the call targets. The two numberings coincide there by construction and
// diverge elsewhere, so they must stay separate APIs.
constexpr uint16_t NATIVE_SCDA_MARKER_OPCODE = 0x001C;
// Prologue opcode. Present in SCPT records only; QUST/INFO inline scripts do
// not carry it, so a decoder must never require it.
constexpr uint16_t NATIVE_SCDA_PROLOGUE_OPCODE = 0x001D;

// Typed operand token kinds used by the general [u16 argc][tokens] framing.
enum class NativeTokenKind : uint8_t {
    Variable,   // [type char][u16 index]  (f/s/r/l/G)
    Integer,    // 'n' + u32
    Double,     // 'z' + f64
    Function,   // 'X' + u16 function id + u16 argument byte count
    BareU16,    // any leading byte that is not a type char
    Axis,       // One byte axis selector ('X'/'Y'/'Z') in an argument list
    String,     // [u16 byteLength][bytes]
    Text,       // ASCII operator text inside an expression
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
    std::string text;           // String literal or operator text

    // A reference property operand is [0x72][u16 refSlot] followed by an
    // optional [0x73][u16 shortIndex]. The pair counts as one operand, so the
    // member token is folded into the reference token and marked here.
    bool hasMember = false;
    uint16_t memberIndex = 0;

    // Function arguments. The nested list carries its own declared count, and
    // hasImplicitSelf is set when it holds one token more than declared.
    std::vector<NativeToken> arguments;
    uint16_t declaredArgumentCount = 0;
    bool hasImplicitSelf = false;
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
    std::vector<NativeToken> tokens;  // Set targets and command arguments
    uint16_t declaredArgumentCount = 0;
    bool hasImplicitSelf = false;     // Leading token is the call reference

    // Decoded expression. expressionDecoded is false when the payload carried no
    // expression or when it could not be tokenized.
    std::vector<NativeToken> expressionTokens;
    bool expressionDecoded = false;

    // String family payloads (MessageBox, Message and the rename commands).
    std::string text;
    uint16_t formatArgumentCount = 0;
    std::vector<NativeToken> formatTokens;
    uint16_t buttonCount = 0;
    std::vector<std::string> buttonTexts;

    // Set when the payload could not be framed. The raw bytes stay in payload so
    // the caller can report the exact opcode and offset instead of guessing.
    bool framingFailed = false;

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

// Tokenizes a reverse Polish expression. The whole buffer must be consumed by
// tokens, separators and operator text; a truncated operand is an error.
bool decodeNativeExpression(const uint8_t* data, size_t size,
                            std::vector<NativeToken>& out, std::string& error);

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

// One entry of a script's reference table, in the order the slots are numbered.
struct NativeReferenceEntry {
    NativeReferenceKind kind = NativeReferenceKind::Scro;
    uint32_t formId = 0;       // Valid when kind == Scro
    uint32_t variableIndex = 0; // SLSD/SCVR variable index when kind == LocalRef
};

// Resolves a 1-based index into a table, the shape both selector spaces share.
//
// The `r`/`Z` slot space and the `0x001C` call selector space are different
// tables, but they agree on the addressing rule: index 0 is not a valid
// selector, and an index past the end is an explicit error rather than a
// guessed reference. Keeping that rule in one place means the two spaces cannot
// drift apart on the boundary they share.
bool resolveNativeTableIndex(uint16_t index,
                             size_t tableSize,
                             const char* spaceName,
                             size_t& out,
                             std::string& error);

// Resolves a 1-based `r`/`Z` slot against a script's reference table.
//
// The table is the SCPT record's ref subrecords in file order: slot k is the
// k-th SCRV or SCRO subrecord of the record, whichever kind it is. SCRV carries
// a u32 SLSD/SCVR variable index and SCRO carries a FormID, and the two kinds
// interleave freely because the compiler emits them in the order it needs them.
// The decisive structural check is SCHR[4], which equals the SCRV count plus the
// SCRO count in 2,393 of 2,393 records: it is the slot count itself.
//
// A fixed "SCRO first, then locals" rule is wrong. In
// XPXirethard01TrapButton01SCRIPT the file order is SCRV(myParent), SCRO x7,
// SCRV(mySelf), so myParent is slot 1 and mySelf is slot 9; a fixed rule would
// put myParent at slot 8 and name the wrong object for every selector.
//
// A slot of 0, or one beyond the table, is an explicit error rather than a
// guessed reference. This is the `r`/`Z` space only: a `0x001C` call selector is
// a different space and must be resolved with resolveNativeCallTarget instead.
bool resolveNativeReferenceSlot(uint16_t slot,
                                const std::vector<NativeReferenceEntry>& entries,
                                NativeReferenceSlot& out,
                                std::string& error);

// Resolves a `0x001C` call selector.
//
// Selectors are numbered in first-appearance order over the call targets the
// bytecode actually uses, so selector 1 is the target of the first `Ref.command`
// in the stream, not SCRO[0]. The mapping is therefore a property of the decoded
// instruction stream, not of the SCRO list, and cannot be derived from the
// reference table alone. The caller supplies the targets it has already seen, in
// first-appearance order; a selector past the end of that list is an explicit
// error rather than a guessed reference.
bool resolveNativeCallTarget(uint16_t selector,
                             const std::vector<uint32_t>& callTargets,
                             uint32_t& out,
                             std::string& error);

// MoveTo (0x109E) argument shapes. The compiler emits exactly four, all framed
// as [u16 argumentCount][tokens]:
//
//   [1][<r>]              MoveTo ref            payload 5
//   [2][<r>][z]           MoveTo ref x          payload 14
//   [4][<r>][z][z][z]     MoveTo ref x y z      payload 32
//   [4][<r>][f][f][z]     MoveTo ref x y z      payload 20
//
// No shape carries a cell reference, so the first token is always the reference
// and the tokens after it are the offsets in source order x, y, z. The compiler
// emits either one offset or three, never two, and the one-offset form is read
// as x with y and z left at zero: the bytecode does not record which axis it is
// and reading it as x is the only self-consistent interpretation.
//
// An offset slot is not always a literal. The payload-20 shape comes from
// sources such as "SEHaskillRef.moveto player x y 0", where x and y are float
// script variables and only z is a literal. In this grammar 'f' introduces a
// 3-byte float variable reference, not a 4-byte float, so the tokens are
// exposed for the VM to evaluate and the literal arrays are filled in only for
// slots the compiler actually wrote as a numeric literal.
struct NativeMoveToArguments {
    bool valid = false;
    bool hasOffsets = false;   // false for the [1][<r>] form
    size_t offsetCount = 0;    // 0, 1 or 3
    // Offset slots in source order x, y, z. The pointers alias the decoded
    // instruction's token vector, so they stay valid only while it does.
    const NativeToken* offsetTokens[3] = {nullptr, nullptr, nullptr};
    // Set for slots the compiler wrote as a numeric literal (z / n / bare u16),
    // with the value in offsetLiterals. Cleared for variable references.
    bool offsetIsLiteral[3] = {false, false, false};
    float offsetLiterals[3] = {0.0f, 0.0f, 0.0f};
};

// Extracts the MoveTo arguments from a decoded instruction. Returns false when
// the instruction is not a MoveTo or its payload does not match one of the four
// shapes.
bool decodeNativeMoveToArguments(const NativeInstruction& instruction,
                                 NativeMoveToArguments& out);

} // namespace script
} // namespace oblivion
