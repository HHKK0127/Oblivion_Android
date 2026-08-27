#pragma once

#include <cstdint>
#include <string>
#include <variant>

// ============================================================================
// Oblivion Script VM - Opcode Definitions
// Based on reverse-engineered Oblivion bytecode format
// ============================================================================

namespace oblivion {
namespace script {

// ============================================================================
// Script types (from SCHR header)
// ============================================================================
enum class ScriptType : uint32_t {
    Object = 0,     // Attached to objects (containers, doors, activators)
    Quest  = 1,     // Quest scripts (run when quest is active)
    Magic  = 2,     // Magic effect scripts (spells, enchantments)
};

// ============================================================================
// Script value variant (RPN stack element)
// ============================================================================
struct ScriptValue {
    enum class Type : uint8_t {
        Integer = 0,
        Float   = 1,
        String  = 2,
        Ref     = 3,    // FormID reference
    };

    Type type = Type::Integer;
    int32_t intVal = 0;
    float floatVal = 0.0f;
    std::string strVal;
    uint32_t refVal = 0;    // FormID

    // Factory helpers
    static ScriptValue makeInt(int32_t v) {
        ScriptValue sv;
        sv.type = Type::Integer;
        sv.intVal = v;
        return sv;
    }
    static ScriptValue makeFloat(float v) {
        ScriptValue sv;
        sv.type = Type::Float;
        sv.floatVal = v;
        return sv;
    }
    static ScriptValue makeString(const std::string& v) {
        ScriptValue sv;
        sv.type = Type::String;
        sv.strVal = v;
        return sv;
    }
    static ScriptValue makeRef(uint32_t formID) {
        ScriptValue sv;
        sv.type = Type::Ref;
        sv.refVal = formID;
        return sv;
    }

    // Coerce to numeric
    float toFloat() const {
        if (type == Type::Float) return floatVal;
        if (type == Type::Integer) return static_cast<float>(intVal);
        return 0.0f;
    }
    int32_t toInt() const {
        if (type == Type::Integer) return intVal;
        if (type == Type::Float) return static_cast<int32_t>(floatVal);
        if (type == Type::Ref) return static_cast<int32_t>(refVal);
        return 0;
    }
    bool isTruthy() const {
        return toInt() != 0;
    }
};

// ============================================================================
// Opcode definitions
// ============================================================================
enum class Opcode : uint16_t {
    // Control flow
    STOP        = 0x0000,   // End script execution

    // Arithmetic (pop 2, push 1)
    ADD         = 0x0001,   // a + b
    SUB         = 0x0002,   // a - b
    MUL         = 0x0003,   // a * b
    DIV         = 0x0004,   // a / b
    MOD         = 0x0005,   // a % b

    // Unary
    NEG         = 0x0006,   // -a

    // Comparison (pop 2, push int 0/1)
    CMP_LT      = 0x0007,   // a < b
    CMP_LE      = 0x0008,   // a <= b
    CMP_GT      = 0x0009,   // a > b
    CMP_GE      = 0x000A,   // a >= b
    CMP_EQ      = 0x000B,   // a == b
    CMP_NE      = 0x000C,   // a != b

    // Logical (pop 2, push int 0/1)
    AND         = 0x000D,   // a && b
    OR          = 0x000E,   // a || b
    NOT         = 0x000F,   // !a

    // Control flow
    JUMP        = 0x0010,   // Unconditional jump (arg: uint32 offset)
    JUMP_Z      = 0x0011,   // Jump if top of stack is zero (arg: uint32 offset)

    // Stack operations
    PUSH_INT    = 0x0012,   // Push integer literal (arg: int32)
    PUSH_FLOAT  = 0x0013,   // Push float literal (arg: float)
    PUSH_STRING = 0x0014,   // Push string literal (arg: null-terminated string)
    PUSH_REF    = 0x0015,   // Push reference (arg: uint32 formID)
    POP         = 0x0016,   // Discard top of stack
    DUP         = 0x0017,   // Duplicate top of stack

    // Variable access
    GET_LOCAL   = 0x0020,   // Push local variable (arg: uint16 index)
    SET_LOCAL   = 0x0021,   // Pop and store to local (arg: uint16 index)
    GET_GLOBAL  = 0x0022,   // Push global variable (arg: uint32 formID)
    SET_GLOBAL  = 0x0023,   // Pop and store to global (arg: uint32 formID)

    // String operations
    STR_CAT     = 0x0030,   // Concatenate strings
    STR_LEN     = 0x0031,   // String length
    STR_SUB     = 0x0032,   // Substring

    // Reference operations
    GET_SELF    = 0x0040,   // Push reference to calling object
    GET_TARGET  = 0x0041,   // Push reference to target (activated, etc.)

    // Function call (arg: uint16 function ID, followed by arg count)
    CALL        = 0x1000,
};

// ============================================================================
// Game function IDs (used with CALL opcode)
// ============================================================================
enum class FunctionID : uint16_t {
    // Tier 1 - Must implement
    SetStage        = 0x0001,
    GetStage        = 0x0002,
    AddItem         = 0x0003,
    RemoveItem      = 0x0004,
    GetItemCount    = 0x0005,
    Enable          = 0x0006,
    Disable         = 0x0007,
    Activate        = 0x0008,
    GetDistance     = 0x0009,
    SetPos          = 0x000A,
    GetPos          = 0x000B,
    Message         = 0x000C,
    MessageBox      = 0x000D,

    // Tier 2 - Common
    GetSelf         = 0x0010,
    GetPlayer       = 0x0011,
    Set             = 0x0012,
    Get             = 0x0013,
    Random          = 0x0014,
    Resurrect       = 0x0015,
    PlaceAtMe       = 0x0016,
    MoveTo          = 0x0017,
    Lock            = 0x0018,
    Unlock          = 0x0019,
    GetLockLevel    = 0x001A,
    IsLocked        = 0x001B,
    GetHealth       = 0x001C,
    SetHealth       = 0x001D,
    GetLevel        = 0x001E,
    SetLevel        = 0x001F,
    IsDead          = 0x0020,
    IsInCombat      = 0x0021,
    GetCombatTarget = 0x0022,
    StartCombat     = 0x0023,
    StopCombat      = 0x0024,
    GetAV           = 0x0025,
    SetAV           = 0x0026,
    ModAV           = 0x0027,
    GetBaseAV       = 0x0028,
    AddSpell        = 0x0029,
    RemoveSpell     = 0x002A,
    HasSpell        = 0x002B,
    Cast            = 0x002C,
    IsGhost         = 0x002D,
    SetGhost        = 0x002E,
    IsEssential     = 0x002F,
    SetEssential    = 0x0030,
    IsPlayer        = 0x0031,
    GetSex          = 0x0032,
    GetRace         = 0x0033,
    GetClass        = 0x0034,
    GetFactionRank  = 0x0035,
    SetFactionRank  = 0x0036,
    IsInFaction     = 0x0037,
    GetParentCell   = 0x0038,
    IsInInterior    = 0x0039,
    GetOpenState    = 0x003A,
    IsOpen          = 0x003B,
    GetValue        = 0x003C,
    GetWeight       = 0x003D,
    GetOwner        = 0x003E,
    SetOwner        = 0x003F,
    Say             = 0x0040,
    SayTo           = 0x0041,
    Show            = 0x0042,
    Wait            = 0x0043,
    PlayGroup       = 0x0044,
    LoopGroup       = 0x0045,
    SetUnconscious  = 0x0046,
    IsUnconscious   = 0x0047,
    IsWeaponOut     = 0x0048,
    IsSneaking      = 0x0049,
    IsAlerted       = 0x004A,
    IsAnimal        = 0x004B,
    IsCreature      = 0x004C,
    IsHumanoid      = 0x004D,
    GetPCLevel      = 0x004E,
    GetPCCell       = 0x004F,
    GetCurrentTime  = 0x0050,
    GetCurrentDay   = 0x0051,
    GetCurrentMonth = 0x0052,
    GetCurrentYear  = 0x0053,
    GetWeather      = 0x0054,
    IsRaining       = 0x0055,
    IsSnowing       = 0x0056,
    GetFame         = 0x0057,
    GetInfamy       = 0x0058,
    GetPCFame       = 0x0059,
    GetPCInfamy     = 0x005A,
    GetRegion       = 0x005B,
    GetRelationship  = 0x005C,
    SetRelationship  = 0x005D,
    GetWeapon       = 0x005E,
    GetArmor        = 0x005F,
    IsDetected      = 0x0060,
    GetDetectionLevel = 0x0061,
    IsInjured       = 0x0062,
    IsActivated     = 0x0063,
    GetActivator    = 0x0064,
    IsCellOwner     = 0x0065,
    GetStartLocation = 0x0066,
    GetEnteringLocation = 0x0067,
    IsInMyCell      = 0x0068,
    GetInCell       = 0x0069,
    IsInExterior    = 0x006A,
    GetDaysInMonth  = 0x006B,
    GetSpell        = 0x006C,
    GetSpellCount   = 0x006D,
    GetNthSpell     = 0x006E,
    IsImmobile      = 0x006F,
    SetImmobile     = 0x0070,
    IsInvulnerable  = 0x0071,
    SetInvulnerable = 0x0072,
    IsPlayerTeammate = 0x0073,
    GetPlayerSkill  = 0x0074,
    GetPCCount      = 0x0075,
    IsPCAmount      = 0x0076,
    GetPCLocation   = 0x0077,
    IsPCLocation    = 0x0078,
};

// ============================================================================
// Instruction structure
// ============================================================================
struct Instruction {
    Opcode opcode;
    uint16_t argLength;      // Length of argument data in bytes
    const uint8_t* argData;  // Pointer to argument data (not owned)
};

// ============================================================================
// Limits
// ============================================================================
namespace limits {
    constexpr int MAX_INSTRUCTIONS_PER_FRAME = 1000;
    constexpr int MAX_GLOBAL_VARIABLES = 10000;
    constexpr int MAX_RECURSION_DEPTH = 16;
    constexpr int MAX_STACK_SIZE = 256;
    constexpr int MAX_LOCAL_VARIABLES = 64;
    constexpr int MAX_REFERENCES = 32;
}

} // namespace script
} // namespace oblivion
