#include "player_character_state.h"

#include <cstring>

namespace game {

// Pin the byte-offset map: any gap-size mistake breaks the build here.
static_assert(sizeof(PlayerCharacterState) == kPlayerCharacterBlockSize,
              "PlayerCharacterState must match the native 0x804-byte block");

#define PCS_ASSERT_OFFSET(field, off) \
    static_assert(offsetof(PlayerCharacterState, field) == (off), \
                  #field " must sit at byte offset " #off)

PCS_ASSERT_OFFSET(vtableSlot, 0x000);
PCS_ASSERT_OFFSET(slot018, 0x018);
PCS_ASSERT_OFFSET(posX, 0x02C);
PCS_ASSERT_OFFSET(posY, 0x030);
PCS_ASSERT_OFFSET(posZ, 0x034);
PCS_ASSERT_OFFSET(afterMoveFlag, 0x045);
PCS_ASSERT_OFFSET(movingFlag, 0x04B);
PCS_ASSERT_OFFSET(subObject058, 0x058);
PCS_ASSERT_OFFSET(slot05C, 0x05C);
PCS_ASSERT_OFFSET(slot068, 0x068);
PCS_ASSERT_OFFSET(tickCounter110, 0x110);
PCS_ASSERT_OFFSET(byte115, 0x115);
PCS_ASSERT_OFFSET(dialogueRecursionGuard, 0x116);
PCS_ASSERT_OFFSET(scaleValue11C, 0x11C);
PCS_ASSERT_OFFSET(saveInProgressFlag12C, 0x12C);
PCS_ASSERT_OFFSET(tempPointer1E0, 0x1E0);
PCS_ASSERT_OFFSET(dynamicBand, 0x200);
PCS_ASSERT_OFFSET(viewStateFlag588, 0x588);
PCS_ASSERT_OFFSET(byte589, 0x589);
PCS_ASSERT_OFFSET(byte58A, 0x58A);
PCS_ASSERT_OFFSET(byte58B, 0x58B);
PCS_ASSERT_OFFSET(combatHandle590, 0x590);
PCS_ASSERT_OFFSET(byte5A8, 0x5A8);
PCS_ASSERT_OFFSET(byte5A9, 0x5A9);
PCS_ASSERT_OFFSET(subAlloc5B0, 0x5B0);
PCS_ASSERT_OFFSET(modeFlag5C0, 0x5C0);
PCS_ASSERT_OFFSET(refCountedObject5D8, 0x5D8);
PCS_ASSERT_OFFSET(dword5DC, 0x5DC);
PCS_ASSERT_OFFSET(objectPointer5F4, 0x5F4);
PCS_ASSERT_OFFSET(timeElapsedCounter6B0, 0x6B0);
PCS_ASSERT_OFFSET(difficulty734, 0x734);
PCS_ASSERT_OFFSET(inCombatFlag738, 0x738);
PCS_ASSERT_OFFSET(subAlloc790, 0x790);
PCS_ASSERT_OFFSET(counter7A0, 0x7A0);
PCS_ASSERT_OFFSET(newGameFlag7F8, 0x7F8);
PCS_ASSERT_OFFSET(slot800, 0x800);

#undef PCS_ASSERT_OFFSET

// The dynamic band covers dword indices 0x81..0x15B = bytes +0x204..+0x56C.
// +0x200 itself is the array header slot (native allocation bookkeeping);
// the band member starts at 0x200 and index 0x81 maps to (0x81 - 0x81) * 4
// inside the band payload at +0x204.
namespace {

constexpr std::size_t kBandHeaderBytes = 4;  // +0x200 dword before payload

bool inDynamicBand(uint32_t index) {
    return index >= kDynamicBandFirstIndex && index <= kDynamicBandLastIndex;
}

uint32_t* bandElement(PlayerCharacterState* state, uint32_t index) {
    // Payload starts at dynamicBand + 4 (byte 0x204); element for idx 0x81
    // is the first payload dword.
    return reinterpret_cast<uint32_t*>(
               state->dynamicBand + kBandHeaderBytes) +
           (index - kDynamicBandFirstIndex);
}

} // namespace

std::unique_ptr<PlayerCharacterState>& PlayerCharacterGlobal::block() {
    static std::unique_ptr<PlayerCharacterState> instance;
    return instance;
}

std::unique_ptr<uint8_t[]>& PlayerCharacterGlobal::alloc058() {
    static std::unique_ptr<uint8_t[]> instance;
    return instance;
}

std::unique_ptr<uint8_t[]>& PlayerCharacterGlobal::alloc5B0() {
    static std::unique_ptr<uint8_t[]> instance;
    return instance;
}

std::unique_ptr<uint8_t[]>& PlayerCharacterGlobal::alloc790() {
    static std::unique_ptr<uint8_t[]> instance;
    return instance;
}

PlayerCharacterState* PlayerCharacterGlobal::create() {
    // Native creating callers only allocate when the global is null
    // (FUN_00406360 / FUN_00464440), so a second call is idempotent.
    if (block()) {
        return block().get();
    }

    // new(0x804) + value-init: the ctor zero-loops the dynamic band, the
    // remaining untouched bytes are indeterminate natively; we zero-init
    // deterministically instead.
    block() = std::make_unique<PlayerCharacterState>();
    PlayerCharacterState* state = block().get();

    // Ctor FUN_0066A740 byte inits. The dword sets (+0x018/+0x05C/+0x068/
    // +0x788/+0x800) and vtable store are native-identity details the port
    // does not need; difficulty defaults to 1.0f via the member initializer.
    state->byte115 = 1;
    state->byte5A9 = 1;
    // dialogueRecursionGuard and the 0x588..0x58B bytes are already 0 from
    // value-init, matching the ctor.

    // Sub-allocations. The native ctor stores the returned pointers into
    // +0x058/+0x5B0/+0x790; those slots stay 0 here because the manager
    // owns the buffers (a 32-bit slot cannot hold a 64-bit pointer).
    alloc058() = std::make_unique<uint8_t[]>(0x2EC);
    std::memset(alloc058().get(), 0, 0x2EC);

    alloc5B0() = std::make_unique<uint8_t[]>(0x54);
    std::memset(alloc5B0().get(), 0, 0x54);

    // Native: malloc(0x94) then memset(ptr, 0x25, 0x25 * 4).
    alloc790() = std::make_unique<uint8_t[]>(0x94);
    std::memset(alloc790().get(), 0x25, 0x25 * 4);

    return state;
}

void PlayerCharacterGlobal::destroy() {
    // FUN_004492E0 releases the sub-objects before deleting the block.
    alloc790().reset();
    alloc5B0().reset();
    alloc058().reset();
    block().reset();
}

PlayerCharacterState* PlayerCharacterGlobal::get() {
    return block().get();
}

bool PlayerCharacterGlobal::isValid() {
    return block() != nullptr;
}

uint32_t PlayerCharacterGlobal::dynamicBandAt(uint32_t index) {
    if (!isValid() || !inDynamicBand(index)) {
        return 0;
    }
    uint32_t value = 0;
    std::memcpy(&value, bandElement(block().get(), index), sizeof(value));
    return value;
}

void PlayerCharacterGlobal::setDynamicBandAt(uint32_t index, uint32_t value) {
    if (!isValid() || !inDynamicBand(index)) {
        return;
    }
    std::memcpy(bandElement(block().get(), index), &value, sizeof(value));
}

} // namespace game
