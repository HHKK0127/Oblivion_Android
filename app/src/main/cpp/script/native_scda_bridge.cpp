#include "native_scda_bridge.h"

#include <string>

namespace oblivion {
namespace script {

namespace {

// The native opcode space and the synthetic FunctionID space are separate
// numbering schemes. This table is the only place that relates them, and it
// lists an entry only where the two names agree exactly.
struct NativeFunctionMapping {
    uint16_t opcode;
    FunctionID functionId;
};

constexpr NativeFunctionMapping kNativeFunctionMappings[] = {
    {0x1000, FunctionID::MessageBox},
    {0x1002, FunctionID::AddItem},
    {0x1003, FunctionID::SetEssential},
    {0x1007, FunctionID::SetPos},
    {0x100D, FunctionID::Activate},
    {0x1013, FunctionID::PlayGroup},
    {0x1016, FunctionID::StartCombat},
    {0x1017, FunctionID::StopCombat},
    {0x101C, FunctionID::AddSpell},
    {0x101D, FunctionID::RemoveSpell},
    {0x101E, FunctionID::Cast},
    {0x1021, FunctionID::Enable},
    {0x1022, FunctionID::Disable},
    {0x1025, FunctionID::PlaceAtMe},
    {0x1033, FunctionID::Say},
    {0x1034, FunctionID::SayTo},
    {0x1036, FunctionID::StartQuest},
    {0x1037, FunctionID::StopQuest},
    {0x1039, FunctionID::SetStage},
    {0x1052, FunctionID::RemoveItem},
    {0x1058, FunctionID::AddTopic},
    {0x1059, FunctionID::Message},
    {0x1071, FunctionID::CompleteQuest},
    {0x1072, FunctionID::Lock},
    {0x1073, FunctionID::Unlock},
    {0x1089, FunctionID::SetFactionRank},
    {0x108C, FunctionID::Resurrect},
    {0x109E, FunctionID::MoveTo},
    {0x10EC, FunctionID::SetGhost},
    {0x10F1, FunctionID::SetUnconscious},
    {0x1133, FunctionID::SetLevel},
};

// Converts one decoded token into a ScriptValue. A variable token names a slot
// rather than a value, so it is read from the running VM's store; literals and
// strings carry their value directly.
ScriptValue tokenToValue(const NativeCommandContext& context, const NativeToken& token) {
    switch (token.kind) {
        case NativeTokenKind::Variable:
            if (context.variables != nullptr) {
                return context.variables->get(token.index);
            }
            return ScriptValue::makeInt(0);
        case NativeTokenKind::Integer:
        case NativeTokenKind::BareU16:
            return ScriptValue::makeInt(token.intValue);
        case NativeTokenKind::Double:
            return ScriptValue::makeFloat(static_cast<float>(token.doubleValue));
        case NativeTokenKind::String:
        case NativeTokenKind::Text:
            return ScriptValue::makeString(token.text);
        default:
            return ScriptValue::makeInt(token.intValue);
    }
}

} // namespace

bool mapNativeOpcodeToFunctionId(uint16_t opcode, FunctionID& out) {
    for (const NativeFunctionMapping& mapping : kNativeFunctionMappings) {
        if (mapping.opcode == opcode) {
            out = mapping.functionId;
            return true;
        }
    }
    return false;
}

std::vector<ScriptValue> nativeTokensToArguments(
    const NativeCommandContext& context,
    const std::vector<NativeToken>& tokens) {
    std::vector<ScriptValue> args;
    args.reserve(tokens.size() + 1);

    // A command that carried a selector acts on that reference, and the
    // synthetic handlers expect it as the leading argument.
    if (context.referenceFormId != 0) {
        args.push_back(ScriptValue::makeRef(context.referenceFormId));
    }

    for (const NativeToken& token : tokens) {
        args.push_back(tokenToValue(context, token));
    }
    return args;
}

size_t registerNativeFunctionBridge(NativeScdaVm& vm, ScriptFunctions& functions) {
    size_t registered = 0;
    for (const NativeFunctionMapping& mapping : kNativeFunctionMappings) {
        // A FunctionID that is declared but not implemented has no handler to
        // forward to, so it stays unbridged and the VM counts it as unhandled.
        if (!functions.hasFunction(mapping.functionId)) {
            continue;
        }
        const FunctionID functionId = mapping.functionId;
        vm.registerCommand(
            mapping.opcode,
            [&functions, functionId](NativeCommandContext& context,
                                     const std::vector<NativeToken>& tokens,
                                     ScriptValue& returnValue,
                                     std::string& error) {
                ExecutionContext execution;
                execution.setSelfRef(context.selfRef);
                execution.setTargetRef(context.targetRef);

                const std::vector<ScriptValue> args =
                    nativeTokensToArguments(context, tokens);
                const FunctionResult result =
                    functions.execute(functionId, execution, args);
                if (!result.success) {
                    error = result.errorMessage;
                    return false;
                }
                returnValue = result.returnValue;
                return true;
            });
        ++registered;
    }
    return registered;
}

} // namespace script
} // namespace oblivion
