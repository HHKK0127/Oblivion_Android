#include "native_scda_decoder.h"

#include <cstring>

namespace oblivion {
namespace script {

namespace {

constexpr uint32_t HEADER_SIZE = 4;
constexpr uint32_t MAX_PAYLOAD = 4096;

inline uint16_t readU16(const uint8_t* p) {
    uint16_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

inline uint32_t readU32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

inline double readF64(const uint8_t* p) {
    double v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

bool isVariableTypeChar(uint8_t c) {
    return c == 'f' || c == 's' || c == 'r' || c == 'l' || c == 'G';
}

// A string token is [u16 length][length bytes] with no type byte. Inside an
// argument list it must be tested before the typed tokens, because a length word
// such as 0x0072 reads as the reference variable type char 'r' and would swallow
// the string. The text bytes are printable ASCII, which no typed token payload
// contains, so the test does not misfire on real tokens.
bool looksLikeStringToken(const uint8_t* data, size_t size, uint32_t offset) {
    if (offset + 2 > size) {
        return false;
    }
    const uint16_t length = readU16(data + offset);
    if (length == 0 || offset + 2 + length > size) {
        return false;
    }
    for (uint16_t i = 0; i < length; ++i) {
        const uint8_t c = data[offset + 2 + i];
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

// Axis selectors are single bytes and only appear inside argument lists, never
// inside expressions, where 'X' is always a function call. 'X' itself is only an
// axis when too few bytes remain for a function token.
bool isAxisChar(uint8_t c, size_t remaining) {
    if (c == 'Y' || c == 'Z') {
        return true;
    }
    return c == 'X' && remaining < 5;
}

bool decodeArgumentList(const uint8_t* data, size_t size, uint32_t offset,
                        std::vector<NativeToken>& tokens, uint16_t& declaredCount,
                        bool& implicitSelf, std::string& error);

bool readNativeString(const uint8_t* payload, uint32_t size, uint32_t& cursor,
                      std::string& out);

// Decodes one typed token. Returns false when the token cannot be framed.
// allowAxis enables the one byte axis selectors of argument lists; expressions
// and Set targets pass false so that 'X' is always a function call.
bool decodeToken(const uint8_t* data, size_t size, uint32_t offset,
                 bool allowAxis, NativeToken& out) {
    if (offset >= size) {
        return false;
    }

    out = NativeToken{};
    out.offset = offset;
    const uint8_t lead = data[offset];

    // Argument lists carry string tokens, and their length word can look like a
    // typed token, so the string reading is tried first there.
    if (allowAxis && looksLikeStringToken(data, size, offset)) {
        const uint16_t length = readU16(data + offset);
        out.kind = NativeTokenKind::String;
        out.text.assign(reinterpret_cast<const char*>(data + offset + 2), length);
        out.length = 2 + length;
        return true;
    }

    if (isVariableTypeChar(lead)) {
        if (offset + 3 > size) {
            return false;
        }
        out.kind = NativeTokenKind::Variable;
        out.typeChar = static_cast<char>(lead);
        out.index = readU16(data + offset + 1);
        out.length = 3;
        return true;
    }

    if (allowAxis && isAxisChar(lead, size - offset)) {
        out.kind = NativeTokenKind::Axis;
        out.typeChar = static_cast<char>(lead);
        out.length = 1;
        return true;
    }

    // Outside an argument list 'Z' is the reference literal: a 1-based index
    // into the owning record's SCRO list.
    if (lead == NATIVE_SCDA_REFERENCE_LITERAL_CHAR) {
        if (offset + 3 > size) {
            return false;
        }
        out.kind = NativeTokenKind::Variable;
        out.typeChar = static_cast<char>(NATIVE_SCDA_REFERENCE_LITERAL_CHAR);
        out.index = readU16(data + offset + 1);
        out.length = 3;
        return true;
    }

    if (lead == 'n') {
        if (offset + 5 > size) {
            return false;
        }
        out.kind = NativeTokenKind::Integer;
        out.intValue = static_cast<int32_t>(readU32(data + offset + 1));
        out.length = 5;
        return true;
    }

    if (lead == 'z') {
        if (offset + 9 > size) {
            return false;
        }
        out.kind = NativeTokenKind::Double;
        out.doubleValue = readF64(data + offset + 1);
        out.length = 9;
        return true;
    }

    if (lead == 'X') {
        if (offset + 5 > size) {
            return false;
        }
        out.kind = NativeTokenKind::Function;
        out.index = readU16(data + offset + 1);
        out.argumentBytes = readU16(data + offset + 3);
        // argumentBytes spans the whole argument list, including the 2 byte
        // count word. Measured over the retail corpus: this reading decodes
        // 11,607 expressions with 1 failure, against 10,904 with 704 failures
        // when the count word is excluded.
        out.length = 5 + out.argumentBytes;
        if (offset + out.length > size) {
            return false;
        }
        if (out.argumentBytes > 0) {
            const uint32_t argsOffset = offset + 5;
            std::string error;
            if (!decodeArgumentList(data, argsOffset + out.argumentBytes, argsOffset,
                                    out.arguments, out.declaredArgumentCount,
                                    out.hasImplicitSelf, error)) {
                return false;
            }
        }
        return true;
    }

    // Any other leading byte is a bare u16 value, such as an actor value code.
    if (offset + 2 > size) {
        return false;
    }
    out.kind = NativeTokenKind::BareU16;
    out.intValue = static_cast<int32_t>(readU16(data + offset));
    out.length = 2;
    return true;
}

// Walks [u16 argc][typed tokens] to the end of the given range and checks the
// resulting token count. A count of argc + 1 means the first token is the call
// reference, which the compiler emits for dotted calls.
//
// A reference property operand is a reference variable followed by the member
// variable it selects, and the pair counts as one operand. The bytecode for a
// property access and for two adjacent operands is identical, so the reading is
// chosen by which one satisfies the declared count: the plain reading first,
// then the folding reading. Folding unconditionally would break the many
// argument lists that pass a reference and a local variable side by side.
bool walkArgumentList(const uint8_t* data, size_t size, uint32_t offset,
                      bool allowAxis, bool foldMembers,
                      std::vector<NativeToken>& tokens, uint16_t& declaredCount,
                      bool& implicitSelf, std::string& error) {
    tokens.clear();
    implicitSelf = false;
    declaredCount = 0;

    if (offset + 2 > size) {
        error = "truncated argument count";
        return false;
    }

    declaredCount = readU16(data + offset);
    uint32_t cursor = offset + 2;

    // The declared count bounds the walk. Reading to the end of the slice would
    // swallow the tokens that follow the argument list, because the slice is
    // sized to the argument bytes and the count word is not part of them.
    const size_t walkLimit = static_cast<size_t>(declaredCount) + 1;
    while (cursor < size && tokens.size() < walkLimit) {
        NativeToken token;
        if (!decodeToken(data, size, cursor, allowAxis, token)) {
            error = "truncated argument token";
            return false;
        }
        cursor += token.length;

        if (foldMembers && token.kind == NativeTokenKind::Variable &&
            token.typeChar == 'r' && cursor + 3 <= size && data[cursor] == 's') {
            NativeToken member;
            if (decodeToken(data, size, cursor, allowAxis, member) &&
                member.kind == NativeTokenKind::Variable) {
                token.hasMember = true;
                token.memberIndex = member.index;
                cursor += member.length;
            }
        }

        tokens.push_back(token);
    }

    if (tokens.size() != declaredCount) {
        error = "argument count " + std::to_string(declaredCount) +
                " does not match " + std::to_string(tokens.size()) + " tokens";
        return false;
    }

    return true;
}

// Decodes [u16 argc][typed tokens] from offset to size.
//
// Axis selectors are one byte and therefore change how many tokens a payload
// yields, while a bare u16 whose value happens to be 0x005A needs the plain
// reading. Both readings exist in the corpus, so prefer the one that satisfies
// the declared count: axis first, because that is what the vanilla argument
// lists use, then plain. Member folding is tried only after both plain readings
// fail, so that adjacent reference and local variable operands keep their own
// tokens.
bool decodeArgumentList(const uint8_t* data, size_t size, uint32_t offset,
                        std::vector<NativeToken>& tokens, uint16_t& declaredCount,
                        bool& implicitSelf, std::string& error) {
    std::string axisError;
    if (walkArgumentList(data, size, offset, true, false, tokens, declaredCount,
                         implicitSelf, axisError)) {
        return true;
    }

    std::string plainError;
    if (walkArgumentList(data, size, offset, false, false, tokens, declaredCount,
                         implicitSelf, plainError)) {
        return true;
    }

    std::string foldError;
    if (walkArgumentList(data, size, offset, true, true, tokens, declaredCount,
                         implicitSelf, foldError)) {
        return true;
    }

    tokens.clear();
    implicitSelf = false;
    error = axisError;
    return false;
}

// Decodes [u16 argc][string] where argc is 1, or 2 when the command also carries
// an explicit reference before the string, as the rename commands do.
bool decodeStringArgument(const uint8_t* payload, uint32_t size, uint32_t& cursor,
                          NativeInstruction& out, std::string& error) {
    if (cursor + 2 > size) {
        error = "truncated string argument count";
        return false;
    }

    const uint16_t argc = readU16(payload + cursor);
    cursor += 2;
    out.declaredArgumentCount = argc;

    if (argc == 2) {
        NativeToken token;
        if (!decodeToken(payload, size, cursor, false, token)) {
            error = "truncated string command reference";
            return false;
        }
        cursor += token.length;
        out.tokens.push_back(token);
        // The reference is the call target, not a declared operand, so the
        // string command carries one implicit self operand.
        out.hasImplicitSelf = true;
    } else if (argc != 1) {
        error = "unexpected string argument count " + std::to_string(argc);
        return false;
    }

    if (!readNativeString(payload, size, cursor, out.text)) {
        error = "truncated string literal";
        return false;
    }
    return true;
}

// Decodes [u16 count][count tokens], the format argument block of the message
// commands.
bool readTokenBlock(const uint8_t* payload, uint32_t size, uint32_t& cursor,
                    std::vector<NativeToken>& tokens, uint16_t& count,
                    std::string& error) {
    tokens.clear();
    count = 0;
    if (cursor + 2 > size) {
        error = "truncated token block count";
        return false;
    }

    count = readU16(payload + cursor);
    cursor += 2;
    for (uint16_t i = 0; i < count; ++i) {
        NativeToken token;
        if (!decodeToken(payload, size, cursor, false, token)) {
            error = "truncated token block token";
            return false;
        }
        cursor += token.length;
        tokens.push_back(token);
    }
    return true;
}

// Decodes [u16 byteLength][bytes]. The stored length counts the bytes that
// follow, and the text keeps only the part before the terminating NUL.
bool readNativeString(const uint8_t* payload, uint32_t size, uint32_t& cursor,
                      std::string& out) {
    if (cursor + 2 > size) {
        return false;
    }

    const uint16_t length = readU16(payload + cursor);
    if (cursor + 2 + length > size) {
        return false;
    }

    const char* text = reinterpret_cast<const char*>(payload + cursor + 2);
    size_t textLength = length;
    while (textLength > 0 && text[textLength - 1] == '\0') {
        --textLength;
    }
    out.assign(text, textLength);
    cursor += 2 + length;
    return true;
}

// Decodes the payload of the message and rename commands.
bool decodeStringCommand(uint16_t opcode, const uint8_t* payload, uint32_t size,
                         NativeInstruction& out, std::string& error) {
    uint32_t cursor = 0;
    if (!decodeStringArgument(payload, size, cursor, out, error)) {
        return false;
    }

    switch (opcode) {
        case 0x1000: {  // MessageBox: text, format arguments, buttons
            if (!readTokenBlock(payload, size, cursor, out.formatTokens,
                                out.formatArgumentCount, error)) {
                return false;
            }
            if (cursor + 2 > size) {
                error = "truncated button count";
                return false;
            }
            out.buttonCount = readU16(payload + cursor);
            cursor += 2;
            for (uint16_t i = 0; i < out.buttonCount; ++i) {
                // Each button carries a leading marker word that is always 1,
                // followed by the usual length-prefixed string.
                if (cursor + 2 > size) {
                    error = "truncated button marker";
                    return false;
                }
                if (readU16(payload + cursor) != 1) {
                    error = "unexpected button marker";
                    return false;
                }
                cursor += 2;
                std::string button;
                if (!readNativeString(payload, size, cursor, button)) {
                    error = "truncated button text";
                    return false;
                }
                out.buttonTexts.push_back(button);
            }
            break;
        }
        case 0x1059: {  // Message: text, format arguments, u32 0
            if (!readTokenBlock(payload, size, cursor, out.formatTokens,
                                out.formatArgumentCount, error)) {
                return false;
            }
            if (cursor + 4 != size || readU32(payload + cursor) != 0) {
                error = "message payload is not terminated by a zero word";
                return false;
            }
            cursor += 4;
            break;
        }
        case 0x114D: {  // text followed by four zero bytes
            if (cursor + 4 != size || readU32(payload + cursor) != 0) {
                error = "string payload is not terminated by a zero word";
                return false;
            }
            cursor += 4;
            break;
        }
        case 0x1114:  // PlayBinkFile
        case 0x111B:  // explicit reference rename
        case 0x111C:  // implicit self rename
            break;
        default:
            break;
    }

    if (cursor != size) {
        error = "string payload has " + std::to_string(size - cursor) +
                " trailing bytes";
        return false;
    }
    return true;
}

// Tokenizes a reverse Polish expression. Dispatch is by leading byte: a type
// char always introduces a binary operand, 0x20 is a separator, and anything
// else is ASCII operator text running to the next separator or the end. There
// are no bare u16 operands inside expressions, so an operand that does not fit
// is an error rather than text.
bool decodeExpressionTokens(const uint8_t* data, size_t size,
                            std::vector<NativeToken>& out, std::string& error) {
    out.clear();
    uint32_t cursor = 0;
    while (cursor < size) {
        const uint8_t lead = data[cursor];
        if (lead == 0x20) {
            ++cursor;
            continue;
        }

        if (lead == 'X') {
            if (cursor + 5 > size) {
                error = "truncated function call in expression";
                return false;
            }
            NativeToken token;
            if (!decodeToken(data, size, cursor, false, token)) {
                error = "truncated function arguments in expression";
                return false;
            }
            out.push_back(token);
            cursor += token.length;
            continue;
        }

        if (isVariableTypeChar(lead) || lead == 'n' || lead == 'z' ||
            lead == NATIVE_SCDA_REFERENCE_LITERAL_CHAR) {
            NativeToken token;
            if (!decodeToken(data, size, cursor, false, token)) {
                error = "truncated operand in expression";
                return false;
            }
            out.push_back(token);
            cursor += token.length;
            continue;
        }

        uint32_t end = cursor;
        while (end < size && data[end] != 0x20) {
            ++end;
        }
        NativeToken token;
        token.kind = NativeTokenKind::Text;
        token.offset = cursor;
        token.length = end - cursor;
        token.text.assign(reinterpret_cast<const char*>(data + cursor), token.length);
        out.push_back(token);
        cursor = end;
    }
    return true;
}

} // namespace

bool decodeNativeExpression(const uint8_t* data, size_t size,
                            std::vector<NativeToken>& out, std::string& error) {
    return decodeExpressionTokens(data, size, out, error);
}

std::string getNativeOpcodeName(uint16_t opcode) {
    switch (opcode) {
        case 0x0010: return "Begin";
        case 0x0011: return "End";
        case 0x0015: return "Set";
        case 0x0016: return "If";
        case 0x0017: return "Else";
        case 0x0018: return "ElseIf";
        case 0x0019: return "EndIf";
        case 0x001C: return "SetCallRef";
        case 0x001D: return "Prologue";
        case 0x001E: return "Return";
        case 0x1000: return "MessageBox";
        case 0x1002: return "AddItem";
        case 0x1003: return "SetEssential";
        case 0x1004: return "Rotate";
        case 0x1007: return "SetPos";
        case 0x1008: return "GetAngle";
        case 0x1009: return "SetAngle";
        case 0x100D: return "Activate";
        case 0x100F: return "SetActorValue";
        case 0x1010: return "ModActorValue";
        case 0x1013: return "PlayGroup";
        case 0x1016: return "StartCombat";
        case 0x1017: return "StopCombat";
        case 0x101C: return "AddSpell";
        case 0x101D: return "RemoveSpell";
        case 0x101E: return "Cast";
        case 0x1021: return "Enable";
        case 0x1022: return "Disable";
        case 0x1025: return "PlaceAtMe";
        case 0x1026: return "PlaySound";
        case 0x1033: return "Say";
        case 0x1034: return "SayTo";
        case 0x1036: return "StartQuest";
        case 0x1037: return "StopQuest";
        case 0x1039: return "SetStage";
        case 0x1052: return "RemoveItem";
        case 0x1053: return "ModDisposition";
        case 0x1055: return "ShowMap";
        case 0x1056: return "StartConversation";
        case 0x1058: return "AddTopic";
        case 0x1059: return "Message";
        case 0x105A: return "SetAlert";
        case 0x105C: return "Look";
        case 0x105D: return "StopLook";
        case 0x105E: return "Evp";
        case 0x1060: return "EnablePlayerControls";
        case 0x1061: return "DisablePlayerControls";
        case 0x1064: return "PickIdle";
        case 0x1071: return "CompleteQuest";
        case 0x1072: return "Lock";
        case 0x1073: return "Unlock";
        case 0x1075: return "SetCrimeGold";
        case 0x1076: return "ModCrimeGold";
        case 0x1089: return "SetFactionRank";
        case 0x108B: return "Kill";
        case 0x108C: return "Resurrect";
        case 0x1097: return "AddScriptPackage";
        case 0x1098: return "RemoveScriptPackage";
        case 0x109E: return "MoveTo";
        case 0x10A5: return "RemoveMe";
        case 0x10A8: return "SetFactionReaction";
        case 0x10A9: return "ModFactionReaction";
        case 0x10AD: return "RemoveAllItems";
        case 0x10AE: return "WakeUpPC";
        case 0x10B1: return "SetCombatStyle";
        case 0x10B2: return "PlaySound3D";
        case 0x10BB: return "SetCellPublicFlag";
        case 0x10BF: return "ModAmountSoldStolen";
        case 0x10C0: return "CloseCurrentOblivionGate";
        case 0x10C2: return "SetPCExpelled";
        case 0x10C4: return "SetPCFactionMurder";
        case 0x10C6: return "SetPCFactionSteal";
        case 0x10CC: return "SetDestroyed";
        case 0x10D1: return "SetForceRun";
        case 0x10D8: return "SetDoorDefaultOpen";
        case 0x10D9: return "ShowClassMenu";
        case 0x10DA: return "ShowRaceMenu";
        case 0x10DB: return "ShowBirthsignMenu";
        case 0x10DD: return "SetOpenState";
        case 0x10DE: return "CloseOblivionGate";
        case 0x10E7: return "SetInChargen";
        case 0x10EA: return "ShowSpellmaking";
        case 0x10EB: return "ShowEnchantment";
        case 0x10EC: return "SetGhost";
        case 0x10EE: return "EquipItem";
        case 0x10EF: return "UnequipItem";
        case 0x10F0: return "SetClass";
        case 0x10F1: return "SetUnconscious";
        case 0x10F3: return "SetRestrained";
        case 0x10F8: return "ModPCFame";
        case 0x10FA: return "ModPCInfamy";
        case 0x1101: return "SCAOnActor";
        case 0x1104: return "SetWeather";
        case 0x110C: return "TrapUpdate";
        case 0x110D: return "SetQuestObject";
        case 0x110E: return "ForceAV";
        case 0x110F: return "ModPCSkill";
        case 0x1111: return "EnableFastTravel";
        case 0x1117: return "SetOwnership";
        case 0x1119: return "SetCellOwnership";
        case 0x1114: return "PlayBink";
        case 0x111B: return "SetCellFullName";
        case 0x111C: return "SetActorFullName";
        case 0x1123: return "PlayMagicShaderVisuals";
        case 0x1124: return "PlayMagicEffectVisuals";
        case 0x1125: return "StopMagicShaderVisuals";
        case 0x1127: return "ResetInterior";
        case 0x1129: return "SAA";
        case 0x112A: return "EnableLinkedPathPoints";
        case 0x112B: return "DisableLinkedPathPoints";
        case 0x112D: return "ForceWeather";
        case 0x1133: return "SetLevel";
        case 0x1137: return "ModPCMiscStat";
        case 0x113C: return "SetScale";
        case 0x1141: return "SetNoRumors";
        case 0x1142: return "Dispel";
        case 0x1144: return "TriggerHitShader";
        case 0x1145: return "RefreshTopicList";
        case 0x1146: return "Reset3DState";
        case 0x114A: return "AddAchievement";
        case 0x114D: return "EssentialDeathReload";
        case 0x114E: return "SetShowQuestItems";
        case 0x1150: return "ResetHealth";
        case 0x1151: return "SetIgnoreFriendlyHits";
        case 0x1156: return "SetRigidBodyMass";
        case 0x1158: return "ReleaseWeatherOverride";
        case 0x115C: return "SendTrespassAlarm";
        case 0x115D: return "SetSceneIsComplex";
        case 0x115E: return "Autosave";
        case 0x1164: return "ShowDialogSubtitles";
        case 0x1165: return "ForceCloseOblivionGate";
        case 0x116B: return "PCB";
        case 0x116C: return "SetPlayerInSEWorld";
        case 0x116E: return "PushActorAway";
        case 0x116F: return "SetActorsAI";
        case 0x1170: return "ClearOwnership";
        default: return std::string();
    }
}

std::vector<std::string> getNativeOpcodeAliases(uint16_t opcode) {
    // The compiler emits the short spelling far more often than the long one,
    // so both must be accepted for the same opcode.
    switch (opcode) {
        case 0x100F: return {"setav", "setactorvalue"};
        case 0x1010: return {"modav", "modactorvalue"};
        case 0x1008: return {"getangle"};
        case 0x1009: return {"setangle"};
        case 0x105E: return {"evp", "evaluatepackage"};
        case 0x109E: return {"moveto", "movetomarker"};
        case 0x1104: return {"setweather", "sw"};
        case 0x1123: return {"pms", "playmagicshadervisuals"};
        case 0x1124: return {"pme", "playmagiceffectvisuals"};
        case 0x1125: return {"sms", "stopmagicshadervisuals"};
        case 0x1129: return {"saa", "setactoralpha"};
        case 0x112D: return {"forceweather", "fw"};
        default: return {};
    }
}

bool resolveNativeReferenceSlot(uint16_t slot,
                                const std::vector<NativeReferenceEntry>& entries,
                                NativeReferenceSlot& out,
                                std::string& error) {
    if (slot == 0) {
        error = "reference slot 0 is not a valid selector";
        return false;
    }
    if (slot > entries.size()) {
        error = "reference slot " + std::to_string(slot) +
                " is outside the reference table (bound " +
                std::to_string(entries.size()) + ")";
        return false;
    }
    const NativeReferenceEntry& entry = entries[slot - 1];
    out = NativeReferenceSlot{};
    out.slot = slot;
    out.kind = entry.kind;
    if (entry.kind == NativeReferenceKind::Scro) {
        out.formId = entry.formId;
    } else {
        out.localOrdinal = static_cast<uint16_t>(entry.variableIndex);
    }
    return true;
}

bool resolveNativeCallTarget(uint16_t selector,
                             const std::vector<uint32_t>& callTargets,
                             uint32_t& out,
                             std::string& error) {
    if (selector == 0) {
        error = "call selector 0 is not a valid selector";
        return false;
    }
    if (selector > callTargets.size()) {
        error = "call selector " + std::to_string(selector) +
                " is outside the call target table (bound " +
                std::to_string(callTargets.size()) + ")";
        return false;
    }
    out = callTargets[selector - 1];
    return true;
}

bool decodeNativeMoveToArguments(const NativeInstruction& instruction,
                                 NativeMoveToArguments& out) {
    out = NativeMoveToArguments{};
    if (instruction.opcode != 0x109E) return false;

    const auto& tokens = instruction.tokens;
    // Every shape starts with the reference token, so a payload without one is
    // not a MoveTo the compiler could have emitted.
    if (tokens.empty() || tokens[0].kind != NativeTokenKind::Variable) return false;

    const size_t offsetCount = tokens.size() - 1;
    // The compiler emits one offset or three, never two; [1][<r>] has none.
    if (offsetCount != 0 && offsetCount != 1 && offsetCount != 3) return false;

    out.valid = true;
    out.hasOffsets = offsetCount != 0;
    out.offsetCount = offsetCount;

    for (size_t i = 0; i < offsetCount; ++i) {
        const NativeToken& token = tokens[i + 1];
        out.offsetTokens[i] = &token;
        switch (token.kind) {
            case NativeTokenKind::Double:
                out.offsetIsLiteral[i] = true;
                out.offsetLiterals[i] = static_cast<float>(token.doubleValue);
                break;
            case NativeTokenKind::Integer:
            case NativeTokenKind::BareU16:
                out.offsetIsLiteral[i] = true;
                out.offsetLiterals[i] = static_cast<float>(token.intValue);
                break;
            default:
                // A float script variable such as the x and y of
                // "SEHaskillRef.moveto player x y 0"; the VM resolves it.
                break;
        }
    }
    return true;
}

bool decodeNativeInstruction(const uint8_t* data, size_t size, uint32_t offset,
                             NativeInstruction& out, std::string& error) {
    if (offset + HEADER_SIZE > size) {
        error = "truncated instruction header";
        return false;
    }

    out = NativeInstruction{};
    out.offset = offset;
    out.opcode = readU16(data + offset);
    const uint16_t lengthWord = readU16(data + offset + 2);

    if (out.opcode == NATIVE_SCDA_MARKER_OPCODE) {
        // Call reference selector: the second u16 is a reference index, not a
        // payload length. The 4 bytes are always consumed.
        out.isMarker = true;
        out.referenceIndex = lengthWord;
        out.encodedLength = HEADER_SIZE;
        out.payloadLength = 0;
        return true;
    }

    if (out.opcode == NATIVE_SCDA_PROLOGUE_OPCODE) {
        out.isPrologue = true;
        out.encodedLength = HEADER_SIZE + lengthWord;
        out.payloadLength = lengthWord;
        if (offset + out.encodedLength > size) {
            error = "truncated prologue payload";
            return false;
        }
        out.payload.assign(data + offset + HEADER_SIZE,
                           data + offset + out.encodedLength);
        return true;
    }

    if (lengthWord > MAX_PAYLOAD) {
        error = "implausible payload length " + std::to_string(lengthWord);
        return false;
    }

    out.encodedLength = HEADER_SIZE + lengthWord;
    out.payloadLength = lengthWord;
    if (offset + out.encodedLength > size) {
        error = "truncated payload";
        return false;
    }

    const uint8_t* payload = data + offset + HEADER_SIZE;

    switch (out.opcode) {
        case static_cast<uint16_t>(NativeStructuralOpcode::Begin): {
            out.isStructural = true;
            // The payload always carries blockType and bodyLength. The trailing
            // word and the argument list are optional, so the minimum is 4.
            if (lengthWord < 4) {
                error = "Begin payload shorter than 4 bytes";
                return false;
            }
            out.blockType = readU16(payload);
            out.bodyLength = readU16(payload + 2);
            break;
        }
        case static_cast<uint16_t>(NativeStructuralOpcode::End):
        case static_cast<uint16_t>(NativeStructuralOpcode::EndIf):
        case static_cast<uint16_t>(NativeStructuralOpcode::Return): {
            out.isStructural = true;
            out.isBare = true;
            if (lengthWord != 0) {
                error = "bare structural opcode with payload";
                return false;
            }
            break;
        }
        case static_cast<uint16_t>(NativeStructuralOpcode::Else): {
            out.isStructural = true;
            if (lengthWord != 2) {
                error = "Else payload is not 2 bytes";
                return false;
            }
            out.meta = readU16(payload);
            break;
        }
        case static_cast<uint16_t>(NativeStructuralOpcode::If):
        case static_cast<uint16_t>(NativeStructuralOpcode::ElseIf): {
            out.isStructural = true;
            if (lengthWord < 4) {
                error = "conditional payload shorter than 4 bytes";
                return false;
            }
            out.meta = readU16(payload);
            const uint16_t expressionLength = readU16(payload + 2);
            if (static_cast<uint32_t>(expressionLength) + 4 != lengthWord) {
                error = "conditional expression length does not match payload";
                return false;
            }
            out.expression.assign(payload + 4, payload + lengthWord);
            std::string expressionError;
            out.expressionDecoded =
                decodeExpressionTokens(payload + 4, expressionLength,
                                       out.expressionTokens, expressionError);
            break;
        }
        case static_cast<uint16_t>(NativeStructuralOpcode::Set): {
            out.isStructural = true;
            // Set has two payload shapes. The remote shape writes a variable of
            // another script:
            //   [u8 0x72][u16 refSlot][u8 type][u16 remoteVarIndex][u16 elen][expr]
            // The local shape stores no target count. Its target is one token,
            // or two when the statement writes a member of a reference, and the
            // length equation resolves it: exactly one reading makes
            // targetBytes + 2 + expressionLength equal the payload length.
            // Targets are always variables, which rejects the reading that
            // would swallow the first expression operand as a second target.
            //
            // 0x72 is also the type char of a reference variable, so a local Set
            // whose first target is a reference starts with the same byte and
            // both readings can satisfy the length equation. The remote reading
            // therefore wins whenever the payload starts with 0x72, which is the
            // only reading that keeps the remote variable index and its type
            // char together.
            if (lengthWord >= 8 && payload[0] == NATIVE_SCDA_REMOTE_SET_CHAR) {
                const uint16_t expressionLength = readU16(payload + 6);
                if (expressionLength != 0 &&
                    static_cast<uint32_t>(expressionLength) + 8 == lengthWord) {
                    NativeToken target;
                    target.kind = NativeTokenKind::Variable;
                    target.offset = 1;
                    target.length = 2;
                    target.typeChar = static_cast<char>(payload[3]);
                    target.index = readU16(payload + 4);
                    out.tokens.push_back(target);
                    out.declaredArgumentCount = 1;
                    out.expression.assign(payload + 8, payload + lengthWord);
                    std::string remoteExpressionError;
                    out.expressionDecoded = decodeExpressionTokens(
                        payload + 8, expressionLength, out.expressionTokens,
                        remoteExpressionError);
                    break;
                }
            }
            bool framed = false;
            for (uint16_t targetCount = 2; targetCount >= 1 && !framed; --targetCount) {
                uint32_t cursor = 0;
                std::vector<NativeToken> targets;
                bool targetsOk = true;
                for (uint16_t i = 0; i < targetCount; ++i) {
                    NativeToken token;
                    if (!decodeToken(payload, lengthWord, cursor, false, token) ||
                        token.kind != NativeTokenKind::Variable) {
                        targetsOk = false;
                        break;
                    }
                    cursor += token.length;
                    targets.push_back(token);
                }
                if (!targetsOk || cursor + 2 > lengthWord) {
                    continue;
                }

                const uint16_t expressionLength = readU16(payload + cursor);
                if (expressionLength == 0 ||
                    cursor + 2 + expressionLength != lengthWord) {
                    continue;
                }

                out.tokens = std::move(targets);
                out.declaredArgumentCount = targetCount;
                out.expression.assign(payload + cursor + 2, payload + lengthWord);
                std::string expressionError;
                out.expressionDecoded = decodeExpressionTokens(
                    payload + cursor + 2, expressionLength, out.expressionTokens,
                    expressionError);
                framed = true;
            }
            if (!framed) {
                out.tokens.clear();
                out.declaredArgumentCount = 0;
                out.framingFailed = true;
                out.payload.assign(payload, payload + lengthWord);
            }
            break;
        }
        case 0x1000:  // MessageBox
        case 0x1059:  // Message
        case 0x1114:  // PlayBinkFile
        case 0x111B:  // Rename with an explicit reference
        case 0x111C:  // Rename with an implicit self
        case 0x114D: { // Text with a zero terminator
            std::string stringError;
            if (!decodeStringCommand(out.opcode, payload, lengthWord, out,
                                     stringError)) {
                out.tokens.clear();
                out.text.clear();
                out.formatTokens.clear();
                out.formatArgumentCount = 0;
                out.buttonCount = 0;
                out.buttonTexts.clear();
                out.declaredArgumentCount = 0;
                out.hasImplicitSelf = false;
                out.framingFailed = true;
                out.payload.assign(payload, payload + lengthWord);
            }
            break;
        }
        default: {
            // Command payloads use the same [u16 argc][tokens] framing. When the
            // framing does not hold, keep the raw payload so the caller can
            // report the exact opcode and offset instead of guessing.
            std::string framingError;
            if (lengthWord == 0) {
                out.isBare = true;
            } else if (!decodeArgumentList(payload, lengthWord, 0, out.tokens,
                                           out.declaredArgumentCount,
                                           out.hasImplicitSelf, framingError)) {
                out.tokens.clear();
                out.declaredArgumentCount = 0;
                out.hasImplicitSelf = false;
                out.framingFailed = true;
                out.payload.assign(payload, payload + lengthWord);
            }
            break;
        }
    }

    return true;
}

NativeDecodeResult decodeNativeScda(const std::vector<uint8_t>& bytecode) {
    NativeDecodeResult result;

    uint32_t offset = 0;
    while (offset < bytecode.size()) {
        NativeInstruction instruction;
        std::string error;
        if (!decodeNativeInstruction(bytecode.data(), bytecode.size(), offset,
                                     instruction, error)) {
            result.success = false;
            result.error = error;
            result.errorOffset = offset;
            result.errorOpcode = offset + 2 <= bytecode.size()
                                     ? readU16(bytecode.data() + offset)
                                     : 0;
            return result;
        }

        if (instruction.encodedLength == 0) {
            result.success = false;
            result.error = "zero-length instruction";
            result.errorOffset = offset;
            result.errorOpcode = instruction.opcode;
            return result;
        }

        offset += instruction.encodedLength;
        result.instructions.push_back(std::move(instruction));
    }

    result.success = true;
    return result;
}

} // namespace script
} // namespace oblivion
