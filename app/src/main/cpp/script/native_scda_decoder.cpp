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
    return c == 'f' || c == 's' || c == 'r' || c == 'l';
}

// Decodes one typed token. Returns false when the token cannot be framed.
bool decodeToken(const uint8_t* data, size_t size, uint32_t offset,
                 NativeToken& out) {
    if (offset >= size) {
        return false;
    }

    out = NativeToken{};
    out.offset = offset;
    const uint8_t lead = data[offset];

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
        out.length = 5;
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

// Decodes [u16 argc][typed tokens]. A token count of argc + 1 means the first
// token is the call reference, which the compiler emits for dotted calls.
bool decodeArgumentList(const uint8_t* data, size_t size, uint32_t offset,
                        NativeInstruction& out, std::string& error) {
    if (offset + 2 > size) {
        error = "truncated argument count";
        return false;
    }

    const uint16_t argc = readU16(data + offset);
    out.declaredArgumentCount = argc;
    uint32_t cursor = offset + 2;

    std::vector<NativeToken> tokens;
    while (cursor < size) {
        NativeToken token;
        if (!decodeToken(data, size, cursor, token)) {
            error = "truncated argument token";
            return false;
        }
        tokens.push_back(token);
        cursor += token.length;
    }

    if (tokens.size() != argc && tokens.size() != static_cast<size_t>(argc) + 1) {
        error = "argument count " + std::to_string(argc) +
                " does not match " + std::to_string(tokens.size()) + " tokens";
        return false;
    }

    if (tokens.size() == static_cast<size_t>(argc) + 1) {
        out.hasImplicitSelf = true;
    }

    out.tokens = std::move(tokens);
    return true;
}

} // namespace

std::string getNativeOpcodeName(uint16_t opcode) {
    switch (opcode) {
        case 0x0010: return "Begin";
        case 0x0011: return "End";
        case 0x0015: return "Set";
        case 0x0016: return "If";
        case 0x0017: return "Else";
        case 0x0018: return "ElseIf";
        case 0x0019: return "EndIf";
        case 0x001C: return "Marker";
        case 0x001D: return "Prologue";
        case 0x001E: return "Return";
        case 0x1000: return "MessageBox";
        case 0x1002: return "AddItem";
        case 0x1003: return "SetEssential";
        case 0x1004: return "Rotate";
        case 0x1007: return "SetPos";
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
        case 0x10AD: return "RemoveAllItems";
        case 0x10AE: return "WakeUpPC";
        case 0x10B1: return "SetCombatStyle";
        case 0x10B2: return "PlaySound3D";
        case 0x10BB: return "SetCellPublicFlag";
        case 0x10C0: return "CloseCurrentOblivionGate";
        case 0x10C2: return "SetPCExpelled";
        case 0x10C4: return "SetPCFactionMurder";
        case 0x10C6: return "SetPCFactionSteal";
        case 0x10CC: return "SetDestroyed";
        case 0x10D1: return "SetForceRun";
        case 0x10D8: return "SetDoorDefaultOpen";
        case 0x10DD: return "SetOpenState";
        case 0x10EC: return "SetGhost";
        case 0x10EE: return "EquipItem";
        case 0x10EF: return "UnequipItem";
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
        case 0x1117: return "SetOwnership";
        case 0x1119: return "SetCellOwnership";
        case 0x111B: return "SetCellFullName";
        case 0x111C: return "SetActorFullName";
        case 0x1123: return "PlayMagicShaderVisuals";
        case 0x1124: return "PlayMagicEffectVisuals";
        case 0x1125: return "StopMagicShaderVisuals";
        case 0x1129: return "SAA";
        case 0x112A: return "EnableLinkedPathPoints";
        case 0x112B: return "DisableLinkedPathPoints";
        case 0x112D: return "ForceWeather";
        case 0x1137: return "ModPCMiscStat";
        case 0x113C: return "SetScale";
        case 0x1142: return "Dispel";
        case 0x1144: return "TriggerHitShader";
        case 0x1146: return "Reset3DState";
        case 0x114A: return "AddAchievement";
        case 0x114E: return "SetShowQuestItems";
        case 0x1150: return "ResetHealth";
        case 0x1151: return "SetIgnoreFriendlyHits";
        case 0x1156: return "SetRigidBodyMass";
        case 0x1158: return "ReleaseWeatherOverride";
        case 0x115D: return "SetSceneIsComplex";
        case 0x115E: return "Autosave";
        case 0x1164: return "ShowDialogSubtitles";
        case 0x116B: return "PCB";
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
        out.isMarker = true;
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
            if (lengthWord < 8) {
                error = "Begin payload shorter than 8 bytes";
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
            break;
        }
        case static_cast<uint16_t>(NativeStructuralOpcode::Set): {
            out.isStructural = true;
            if (!decodeArgumentList(payload, lengthWord, 0, out, error)) {
                return false;
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
            } else if (!decodeArgumentList(payload, lengthWord, 0, out, framingError)) {
                out.tokens.clear();
                out.hasImplicitSelf = false;
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
