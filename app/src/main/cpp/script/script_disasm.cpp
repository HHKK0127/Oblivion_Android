#include "script_disasm.h"
#include <sstream>
#include <iomanip>
#include <cstring>

// ============================================================================
// Oblivion Script VM - Disassembler Implementation
// ============================================================================

namespace oblivion {
namespace script {

std::string ScriptDisasm::disassemble(const ScriptData& script) {
    std::ostringstream oss;
    oss << "; Script: " << script.editorID << " (0x" << std::hex << script.formID << std::dec << ")\n";
    oss << "; Type: " << static_cast<int>(script.scriptType) << "\n";
    oss << "; Bytecode: " << script.bytecode.size() << " bytes\n";
    oss << "; Variables: " << script.variables.size() << "\n";
    oss << "; References: " << script.references.size() << "\n";
    oss << ";\n";

    // List variables
    for (const auto& var : script.variables) {
        oss << "; var[" << var.index << "] " << var.name << " : ";
        switch (var.type) {
            case ScriptValue::Type::Integer: oss << "int"; break;
            case ScriptValue::Type::Float: oss << "float"; break;
            case ScriptValue::Type::String: oss << "string"; break;
            case ScriptValue::Type::Ref: oss << "ref"; break;
        }
        oss << "\n";
    }

    // List references
    for (size_t i = 0; i < script.references.size(); ++i) {
        oss << "; ref[" << i << "] = 0x" << std::hex << script.references[i] << std::dec << "\n";
    }

    oss << ";\n";

    // Disassemble instructions
    uint32_t offset = 0;
    while (offset < script.bytecode.size()) {
        oss << std::setw(4) << std::setfill('0') << std::hex << offset << ": ";
        oss << disassembleInstruction(script.bytecode.data(), script.bytecode.size(), offset);
        oss << "\n";

        // Advance to next instruction
        if (offset + 4 > script.bytecode.size()) break;
        uint16_t argLen;
        std::memcpy(&argLen, script.bytecode.data() + offset + 2, 2);
        offset += 4 + argLen;

        // Safety check
        if (argLen > 1024) {
            oss << "; ERROR: suspicious arg length, stopping disassembly\n";
            break;
        }
    }

    return oss.str();
}

std::string ScriptDisasm::disassembleInstruction(const uint8_t* bytecode, size_t size, uint32_t offset) {
    if (offset + 4 > size) {
        return "<truncated>";
    }

    std::ostringstream oss;

    // Read opcode and arg length
    uint16_t rawOp;
    uint16_t argLen;
    std::memcpy(&rawOp, bytecode + offset, 2);
    std::memcpy(&argLen, bytecode + offset + 2, 2);

    Opcode op = static_cast<Opcode>(rawOp);
    oss << getOpcodeName(op);

    // Format arguments based on opcode
    const uint8_t* argData = bytecode + offset + 4;

    switch (op) {
        case Opcode::PUSH_INT:
            if (argLen >= 4) {
                int32_t val;
                std::memcpy(&val, argData, 4);
                oss << " " << val;
            }
            break;
        case Opcode::PUSH_FLOAT:
            if (argLen >= 4) {
                float val;
                std::memcpy(&val, argData, 4);
                oss << " " << std::fixed << std::setprecision(2) << val;
            }
            break;
        case Opcode::PUSH_STRING:
            if (argLen > 0) {
                std::string str(reinterpret_cast<const char*>(argData), argLen);
                if (!str.empty() && str.back() == '\0') str.pop_back();
                oss << " \"" << str << "\"";
            }
            break;
        case Opcode::PUSH_REF:
            if (argLen >= 4) {
                uint32_t ref;
                std::memcpy(&ref, argData, 4);
                oss << " 0x" << std::hex << std::setw(8) << std::setfill('0') << ref;
            }
            break;
        case Opcode::JUMP:
        case Opcode::JUMP_Z:
            if (argLen >= 4) {
                uint32_t target;
                std::memcpy(&target, argData, 4);
                oss << " ->0x" << std::hex << std::setw(4) << std::setfill('0') << target;
            }
            break;
        case Opcode::GET_LOCAL:
        case Opcode::SET_LOCAL:
            if (argLen >= 2) {
                uint16_t idx;
                std::memcpy(&idx, argData, 2);
                oss << " [" << idx << "]";
            }
            break;
        case Opcode::GET_GLOBAL:
        case Opcode::SET_GLOBAL:
            if (argLen >= 4) {
                uint32_t formID;
                std::memcpy(&formID, argData, 4);
                oss << " 0x" << std::hex << std::setw(8) << std::setfill('0') << formID;
            }
            break;
        case Opcode::CALL:
            if (argLen >= 4) {
                uint16_t funcID;
                uint16_t argc;
                std::memcpy(&funcID, argData, 2);
                std::memcpy(&argc, argData + 2, 2);
                oss << " " << getFunctionName(funcID) << "(" << argc << " args)";
            }
            break;
        default:
            if (argLen > 0) {
                oss << " " << formatArgs(argData, argLen);
            }
            break;
    }

    return oss.str();
}

const char* ScriptDisasm::getOpcodeName(Opcode op) {
    switch (op) {
        case Opcode::STOP: return "STOP";
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::MOD: return "MOD";
        case Opcode::NEG: return "NEG";
        case Opcode::CMP_LT: return "CMP_LT";
        case Opcode::CMP_LE: return "CMP_LE";
        case Opcode::CMP_GT: return "CMP_GT";
        case Opcode::CMP_GE: return "CMP_GE";
        case Opcode::CMP_EQ: return "CMP_EQ";
        case Opcode::CMP_NE: return "CMP_NE";
        case Opcode::AND: return "AND";
        case Opcode::OR: return "OR";
        case Opcode::NOT: return "NOT";
        case Opcode::JUMP: return "JUMP";
        case Opcode::JUMP_Z: return "JUMP_Z";
        case Opcode::PUSH_INT: return "PUSH_INT";
        case Opcode::PUSH_FLOAT: return "PUSH_FLOAT";
        case Opcode::PUSH_STRING: return "PUSH_STRING";
        case Opcode::PUSH_REF: return "PUSH_REF";
        case Opcode::POP: return "POP";
        case Opcode::DUP: return "DUP";
        case Opcode::GET_LOCAL: return "GET_LOCAL";
        case Opcode::SET_LOCAL: return "SET_LOCAL";
        case Opcode::GET_GLOBAL: return "GET_GLOBAL";
        case Opcode::SET_GLOBAL: return "SET_GLOBAL";
        case Opcode::STR_CAT: return "STR_CAT";
        case Opcode::STR_LEN: return "STR_LEN";
        case Opcode::STR_SUB: return "STR_SUB";
        case Opcode::GET_SELF: return "GET_SELF";
        case Opcode::GET_TARGET: return "GET_TARGET";
        case Opcode::CALL: return "CALL";
        default: return "UNKNOWN";
    }
}

const char* ScriptDisasm::getFunctionName(uint16_t funcID) {
    switch (static_cast<FunctionID>(funcID)) {
        case FunctionID::SetStage: return "SetStage";
        case FunctionID::GetStage: return "GetStage";
        case FunctionID::AddItem: return "AddItem";
        case FunctionID::RemoveItem: return "RemoveItem";
        case FunctionID::GetItemCount: return "GetItemCount";
        case FunctionID::Enable: return "Enable";
        case FunctionID::Disable: return "Disable";
        case FunctionID::Activate: return "Activate";
        case FunctionID::GetDistance: return "GetDistance";
        case FunctionID::SetPos: return "SetPos";
        case FunctionID::GetPos: return "GetPos";
        case FunctionID::Message: return "Message";
        case FunctionID::MessageBox: return "MessageBox";
        case FunctionID::GetSelf: return "GetSelf";
        case FunctionID::GetPlayer: return "GetPlayer";
        case FunctionID::Set: return "Set";
        case FunctionID::Get: return "Get";
        case FunctionID::Random: return "Random";
        case FunctionID::Resurrect: return "Resurrect";
        case FunctionID::PlaceAtMe: return "PlaceAtMe";
        case FunctionID::MoveTo: return "MoveTo";
        case FunctionID::Lock: return "Lock";
        case FunctionID::Unlock: return "Unlock";
        case FunctionID::GetLockLevel: return "GetLockLevel";
        case FunctionID::IsLocked: return "IsLocked";
        case FunctionID::GetHealth: return "GetHealth";
        case FunctionID::SetHealth: return "SetHealth";
        case FunctionID::GetLevel: return "GetLevel";
        case FunctionID::SetLevel: return "SetLevel";
        case FunctionID::IsDead: return "IsDead";
        case FunctionID::IsInCombat: return "IsInCombat";
        case FunctionID::GetCombatTarget: return "GetCombatTarget";
        case FunctionID::StartCombat: return "StartCombat";
        case FunctionID::StopCombat: return "StopCombat";
        case FunctionID::GetAV: return "GetAV";
        case FunctionID::SetAV: return "SetAV";
        case FunctionID::ModAV: return "ModAV";
        case FunctionID::GetBaseAV: return "GetBaseAV";
        case FunctionID::AddSpell: return "AddSpell";
        case FunctionID::RemoveSpell: return "RemoveSpell";
        case FunctionID::HasSpell: return "HasSpell";
        case FunctionID::Cast: return "Cast";
        case FunctionID::IsGhost: return "IsGhost";
        case FunctionID::SetGhost: return "SetGhost";
        case FunctionID::IsEssential: return "IsEssential";
        case FunctionID::SetEssential: return "SetEssential";
        case FunctionID::IsPlayer: return "IsPlayer";
        case FunctionID::GetSex: return "GetSex";
        case FunctionID::GetRace: return "GetRace";
        case FunctionID::GetClass: return "GetClass";
        case FunctionID::GetFactionRank: return "GetFactionRank";
        case FunctionID::SetFactionRank: return "SetFactionRank";
        case FunctionID::IsInFaction: return "IsInFaction";
        case FunctionID::GetParentCell: return "GetParentCell";
        case FunctionID::IsInInterior: return "IsInInterior";
        case FunctionID::GetOpenState: return "GetOpenState";
        case FunctionID::IsOpen: return "IsOpen";
        case FunctionID::GetValue: return "GetValue";
        case FunctionID::GetWeight: return "GetWeight";
        case FunctionID::GetOwner: return "GetOwner";
        case FunctionID::SetOwner: return "SetOwner";
        case FunctionID::Say: return "Say";
        case FunctionID::SayTo: return "SayTo";
        case FunctionID::Show: return "Show";
        case FunctionID::Wait: return "Wait";
        case FunctionID::PlayGroup: return "PlayGroup";
        case FunctionID::LoopGroup: return "LoopGroup";
        case FunctionID::SetUnconscious: return "SetUnconscious";
        case FunctionID::IsUnconscious: return "IsUnconscious";
        case FunctionID::IsWeaponOut: return "IsWeaponOut";
        case FunctionID::IsSneaking: return "IsSneaking";
        case FunctionID::IsAlerted: return "IsAlerted";
        case FunctionID::IsAnimal: return "IsAnimal";
        case FunctionID::IsCreature: return "IsCreature";
        case FunctionID::IsHumanoid: return "IsHumanoid";
        case FunctionID::GetPCLevel: return "GetPCLevel";
        case FunctionID::GetPCCell: return "GetPCCell";
        case FunctionID::GetCurrentTime: return "GetCurrentTime";
        case FunctionID::GetCurrentDay: return "GetCurrentDay";
        case FunctionID::GetCurrentMonth: return "GetCurrentMonth";
        case FunctionID::GetCurrentYear: return "GetCurrentYear";
        case FunctionID::GetWeather: return "GetWeather";
        case FunctionID::IsRaining: return "IsRaining";
        case FunctionID::IsSnowing: return "IsSnowing";
        case FunctionID::GetFame: return "GetFame";
        case FunctionID::GetInfamy: return "GetInfamy";
        case FunctionID::GetPCFame: return "GetPCFame";
        case FunctionID::GetPCInfamy: return "GetPCInfamy";
        case FunctionID::GetRegion: return "GetRegion";
        case FunctionID::GetRelationship: return "GetRelationship";
        case FunctionID::SetRelationship: return "SetRelationship";
        case FunctionID::GetWeapon: return "GetWeapon";
        case FunctionID::GetArmor: return "GetArmor";
        case FunctionID::IsDetected: return "IsDetected";
        case FunctionID::GetDetectionLevel: return "GetDetectionLevel";
        case FunctionID::IsInjured: return "IsInjured";
        case FunctionID::IsActivated: return "IsActivated";
        case FunctionID::GetActivator: return "GetActivator";
        case FunctionID::IsCellOwner: return "IsCellOwner";
        case FunctionID::GetStartLocation: return "GetStartLocation";
        case FunctionID::GetEnteringLocation: return "GetEnteringLocation";
        case FunctionID::IsInMyCell: return "IsInMyCell";
        case FunctionID::GetInCell: return "GetInCell";
        case FunctionID::IsInExterior: return "IsInExterior";
        case FunctionID::GetDaysInMonth: return "GetDaysInMonth";
        case FunctionID::GetSpell: return "GetSpell";
        case FunctionID::GetSpellCount: return "GetSpellCount";
        case FunctionID::GetNthSpell: return "GetNthSpell";
        case FunctionID::IsImmobile: return "IsImmobile";
        case FunctionID::SetImmobile: return "SetImmobile";
        case FunctionID::IsInvulnerable: return "IsInvulnerable";
        case FunctionID::SetInvulnerable: return "SetInvulnerable";
        case FunctionID::IsPlayerTeammate: return "IsPlayerTeammate";
        case FunctionID::GetPlayerSkill: return "GetPlayerSkill";
        case FunctionID::GetPCCount: return "GetPCCount";
        case FunctionID::IsPCAmount: return "IsPCAmount";
        case FunctionID::GetPCLocation: return "GetPCLocation";
        case FunctionID::IsPCLocation: return "IsPCLocation";
        default: return "Unknown";
    }
}

std::string ScriptDisasm::formatArgs(const uint8_t* data, uint16_t length) {
    std::ostringstream oss;
    oss << "[";
    for (uint16_t i = 0; i < length; ++i) {
        if (i > 0) oss << " ";
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    oss << "]";
    return oss.str();
}

} // namespace script
} // namespace oblivion
