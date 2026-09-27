#pragma once

#include "native_scda_vm.h"
#include "script_functions.h"

#include <cstdint>
#include <vector>

// ============================================================================
// Native SCDA to ScriptFunctions bridge
//
// The native VM executes the compiler's own opcode encoding, while the game
// logic lives behind ScriptFunctions, which is keyed by the synthetic
// FunctionID space. This bridge registers a native command handler for every
// opcode whose name matches a FunctionID, converts the decoded argument tokens
// into ScriptValue arguments, and forwards the call.
//
// Only opcodes with an exact name match are bridged. An opcode without a
// counterpart stays unregistered, so the VM counts it as unhandled and keeps
// running instead of guessing at a meaning.
// ============================================================================

namespace oblivion {
namespace script {

// Maps a native opcode to the FunctionID that implements it, or returns false
// when the opcode has no counterpart in the synthetic function table.
bool mapNativeOpcodeToFunctionId(uint16_t opcode, FunctionID& out);

// One row of the native-to-synthetic relation. The table is the single place
// that relates the two numbering schemes, so the name a native opcode reports
// and the name its FunctionID reports are checked against each other rather
// than maintained in two lists.
struct NativeFunctionMapping {
    uint16_t opcode;
    FunctionID functionId;
};

// The relation itself, in table order. Exposed so a test can assert that the
// relation is a bijection and that both name spaces agree on every row.
const NativeFunctionMapping* nativeFunctionMappings(size_t& count);

// Checks the relation for the invariants the bridge relies on: no opcode and no
// FunctionID appears twice, and every row's native opcode name equals the name
// its FunctionID reports. Returns false and fills `error` on the first
// violation, so a bad row is named rather than silently bridged.
bool validateNativeFunctionMappings(std::string& error);

// Converts a decoded native argument list into ScriptValue arguments. The
// leading call reference, when present, becomes the first argument so that
// handlers see the same shape the synthetic VM produces.
std::vector<ScriptValue> nativeTokensToArguments(
    const NativeCommandContext& context,
    const std::vector<NativeToken>& tokens);

// Registers a forwarding handler for every native opcode that maps to a
// FunctionID. Returns the number of opcodes registered.
size_t registerNativeFunctionBridge(NativeScdaVm& vm, ScriptFunctions& functions);

} // namespace script
} // namespace oblivion
