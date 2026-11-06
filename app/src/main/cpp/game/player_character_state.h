#pragma once

// PlayerCharacter global state (DAT_00B333C4) layout, ported from the
// Windows executable per tmp/re/v72_global_b333c4_map.md.
//
// Native identity:
//   - DAT_00B333C4 is a bss pointer to a 0x804-byte heap block.
//   - Created lazily by FUN_00406360 / FUN_00464440 (null-check -> new ->
//     ctor FUN_0066A740 -> store), destroyed by FUN_004492E0 (release ->
//     delete -> store null). Only 3 sites write the global.
//   - Notation: DAT_00b333c4[N] (int subscript) = byte offset +4N.
//     *(T*)(DAT_00b333c4 + N) = byte offset +N. All offsets below are BYTES.
//
// This port keeps one canonical byte-offset layout so every documented
// consumer (dialogue recursion guard +0x116, Show1stPerson-family +0x588,
// in-combat +0x738, difficulty +0x734, ...) shares a single owner.

#include <cstdint>
#include <cstddef>
#include <memory>

namespace game {

// Size of the native heap block: push 0x804 before the new wrapper.
constexpr std::size_t kPlayerCharacterBlockSize = 0x804;

// Dynamic-indexed dword band zeroed by the ctor: idx 0x81..0x15B
// = bytes +0x204..+0x56C. Exposed as dword indices 0x81..0x15B.
constexpr uint32_t kDynamicBandFirstIndex = 0x81;
constexpr uint32_t kDynamicBandLastIndex = 0x15B;

// Byte-offset layout of the 0x804-byte PlayerCharacter block.
// Reserved gaps carry no documented semantics yet; offsets are pinned by
// static_asserts in the .cpp so the map stays auditable.
struct PlayerCharacterState {
    // +0x000: native vtable pointer (set to &PTR_FUN_00a73a0c by the ctor).
    // The port stores no real vtable; the slot exists for offset parity.
    uint32_t vtableSlot = 0;                 // 0x000
    uint8_t reserved004[0x14]{};             // 0x004..0x017
    uint32_t slot018 = 0;                    // 0x018 ctor dword set
    uint8_t reserved01C[0x10]{};             // 0x01C..0x02B
    float posX = 0.0f;                       // 0x02C  DAT[0xb] Vector3.X
    float posY = 0.0f;                       // 0x030  DAT[0xc] Vector3.Y
    float posZ = 0.0f;                       // 0x034  DAT[0xd] Vector3.Z
    uint8_t reserved038[0x0D]{};             // 0x038..0x044
    uint8_t afterMoveFlag = 0;               // 0x045  RE_EXE_REPORT L5742
    uint8_t reserved046[0x05]{};             // 0x046..0x04A
    uint8_t movingFlag = 0;                  // 0x04B  moving flag
    uint8_t reserved04C[0x0C]{};             // 0x04C..0x057
    uint32_t subObject058 = 0;               // 0x058  0x2EC-byte sub-alloc ptr
    uint32_t slot05C = 0;                    // 0x05C  ctor dword set
    uint8_t reserved060[0x08]{};             // 0x060..0x067
    uint32_t slot068 = 0;                    // 0x068  ctor dword set
    uint8_t reserved06C[0xA4]{};             // 0x06C..0x10F
    uint32_t tickCounter110 = 0;             // 0x110  vs bss cap DAT_00B37D18
    uint8_t reserved114 = 0;                 // 0x114  gap byte (ctor leaves)
    uint8_t byte115 = 0;                     // 0x115  ctor: 1
    uint8_t dialogueRecursionGuard = 0;      // 0x116  Topic_Resolve guard
    uint8_t reserved117[0x05]{};             // 0x117..0x11B
    uint32_t scaleValue11C = 0;              // 0x11C  int->float scale
    uint8_t reserved120[0x0C]{};             // 0x120..0x12B
    uint8_t saveInProgressFlag12C = 0;       // 0x12C  byte flag (300 decimal)
    uint8_t reserved12D[0xB3]{};             // 0x12D..0x1DF
    uint32_t tempPointer1E0 = 0;             // 0x1E0  compare-then-clear temp
    uint8_t reserved1E4[0x1C]{};             // 0x1E4..0x1FF
    uint8_t dynamicBand[0x370]{};            // 0x200..0x56F (dword band above)
    uint8_t reserved570[0x18]{};             // 0x570..0x587
    uint8_t viewStateFlag588 = 0;            // 0x588  Show1stPerson-family
    uint8_t byte589 = 0;                     // 0x589  ctor: 0
    uint8_t byte58A = 0;                     // 0x58A  ctor: 0
    uint8_t byte58B = 0;                     // 0x58B  ctor: 0
    uint8_t reserved58C[0x04]{};             // 0x58C..0x58F
    uint32_t combatHandle590 = 0;            // 0x590  StartCombat clears it
    uint8_t reserved594[0x14]{};             // 0x594..0x5A7
    uint8_t byte5A8 = 0;                     // 0x5A8
    uint8_t byte5A9 = 0;                     // 0x5A9  ctor: 1
    uint8_t reserved5AA[0x06]{};             // 0x5AA..0x5AF
    uint32_t subAlloc5B0 = 0;                // 0x5B0  0x54-byte sub-alloc ptr
    uint8_t reserved5B4[0x0C]{};             // 0x5B4..0x5BF
    uint8_t modeFlag5C0 = 0;                 // 0x5C0  byte mode flag (idx 0x170)
    uint8_t reserved5C1[0x17]{};             // 0x5C1..0x5D7
    uint32_t refCountedObject5D8 = 0;        // 0x5D8  refcounted obj (idx 0x176)
    uint32_t dword5DC = 0;                   // 0x5DC  (idx 0x177)
    uint8_t reserved5E0[0x14]{};             // 0x5E0..0x5F3
    uint32_t objectPointer5F4 = 0;           // 0x5F4  null-check -> vtable+0xD4
    uint8_t reserved5F8[0xB8]{};             // 0x5F8..0x6AF
    uint32_t timeElapsedCounter6B0 = 0;      // 0x6B0  incremented each tick
    uint8_t reserved6B4[0x80]{};             // 0x6B4..0x733
    float difficulty734 = 1.0f;              // 0x734  float difficulty value
    uint8_t inCombatFlag738 = 0;             // 0x738  StartCombat sets 1
    uint8_t reserved739[0x57]{};             // 0x739..0x78F (incl. ctor dword
                                               //  at +0x788, kept raw)
    uint32_t subAlloc790 = 0;                // 0x790  0x94-byte sub-alloc ptr
    uint8_t reserved794[0x0C]{};             // 0x794..0x79F
    uint32_t counter7A0 = 0;                 // 0x7A0  FUN_0064BBC0 increments
    uint8_t reserved7A4[0x54]{};             // 0x7A4..0x7F7 (terminal bytes with
                                               //  no named access per v72 §7)
    uint32_t newGameFlag7F8 = 0;             // 0x7F8  new-game flag (RE_EXE L1719)
    uint8_t reserved7FC[0x04]{};             // 0x7FC..0x7FF
    uint32_t slot800 = 0;                    // 0x800  ctor dword set (idx 0x200)
};

// Dynamic-band dword element access (native DAT_00b333c4[idx], idx 0x81..0x15B).
constexpr uint32_t kDynamicBandDwordCount =
    (kDynamicBandLastIndex - kDynamicBandFirstIndex) + 1;

// Singleton manager mirroring the native global DAT_00B333C4.
class PlayerCharacterGlobal {
public:
    // Null-check -> allocate -> ctor -> store. Idempotent like the native
    // creating callers (FUN_00406360 / FUN_00464440 only allocate when null).
    static PlayerCharacterState* create();

    // Release sub-allocs, delete block, store null (FUN_004492E0).
    static void destroy();

    // The global pointer. Null until create() runs; native consumers all
    // null-check before touching fields.
    static PlayerCharacterState* get();

    // True when the global is live (native: if (DAT_00b333c4 != nullptr)).
    static bool isValid();

    // Read a dynamic-band dword (idx 0x81..0x15B). 0 when invalid/out-of-band.
    static uint32_t dynamicBandAt(uint32_t index);
    // Write a dynamic-band dword. No-op when invalid/out-of-band.
    static void setDynamicBandAt(uint32_t index, uint32_t value);

private:
    static std::unique_ptr<PlayerCharacterState>& block();
    static std::unique_ptr<uint8_t[]>& alloc058();  // 0x2EC bytes
    static std::unique_ptr<uint8_t[]>& alloc5B0();  // 0x54 bytes
    static std::unique_ptr<uint8_t[]>& alloc790();  // 0x94 bytes, memset 0x25*4
};

} // namespace game
