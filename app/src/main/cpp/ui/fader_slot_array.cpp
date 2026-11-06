#include "fader_slot_array.h"

#include <algorithm>
#include <limits>

namespace {

// Original template 0xA688F0 is "Data\Textures\Menus\Faders\%s"; the port
// uses forward slashes for the asset loader.
constexpr const char* kTextureDir = "Data/Textures/Menus/Faders/";

}  // namespace

FaderSlotArray::FaderSlotArray() {
    // .data initials measured at 0x00B12DC0 (RE 48.2). enabled, duration,
    // texture and current all start at 0/NULL in the image.
    slots_[0].type_flag = 1;
    slots_[0].texture_name = "LetterBox.dds";
    slots_[1].type_flag = 1;
    slots_[1].texture_name = "Black.dds";
    slots_[2].type_flag = 0;
    slots_[2].texture_name = "Black.dds";
}

float FaderSlotArray::step(float dt, float duration) {
    if (duration == 0.0f) {
        // x87: 1/0 = +inf, so any dt > 0 clamps instantly; dt == 0 would
        // be NaN, the port leaves current untouched instead.
        return (dt == 0.0f) ? 0.0f : std::numeric_limits<float>::infinity();
    }
    return dt / duration;
}

void FaderSlotArray::update(float dt, uint32_t arg) {
    for (int i = 0; i < kSlotCount; ++i) {
        Slot& s = slots_[i];
        if (s.texture_object == nullptr) continue;  // unloaded slot

        if (s.enabled) {
            // 0x5ADB40: unidentified request hook, slot 2 only, arg != 0.
            if (arg != 0 && i == 2 && slot2_request_hook_) {
                slot2_request_hook_(i);
            }
            if (s.current >= 1.0f) continue;  // fully faded: untouched
            s.current = std::min(1.0f, s.current + step(dt, s.duration));
            if (draw_hook_) draw_hook_(i, s.texture_object, s.current);
        } else if (s.current > 0.0f) {
            s.current = std::max(0.0f, s.current - step(dt, s.duration));
            if (draw_hook_) draw_hook_(i, s.texture_object, s.current);
        } else {
            // current reached 0: release the texture object and empty the
            // slot. The release frame draws nothing.
            if (release_hook_) release_hook_(i, s.texture_object);
            s.texture_object = nullptr;
            s.current = 0.0f;
        }
    }
}

void FaderSlotArray::reset(int slot, int flag) {
    if (!valid(slot)) return;
    Slot& s = slots_[slot];
    if (s.texture_object == nullptr) return;

    const bool latched = menu_latch_query_ ? menu_latch_query_() : false;
    if (flag == 0 && !latched) {
        // Simple disable: the update path fades the texture out over the
        // following frames. +0x10/+0x14 stay untouched here.
        s.enabled = false;
        return;
    }

    // Full reset: release now, whatever current was. +0x00/+0x04 survive
    // (48.2 errata: the report's "+0=0, +4=0" claim is contradicted by the
    // raw disassembly); enabled is not cleared by this path either.
    if (release_hook_) release_hook_(slot, s.texture_object);
    s.current = 0.0f;
    s.texture_object = nullptr;
    if (post_reset_hook_) post_reset_hook_();
}

float FaderSlotArray::current(int slot) const {
    return valid(slot) ? slots_[slot].current : 0.0f;
}

bool FaderSlotArray::is_active(int slot) const {
    return valid(slot) && slots_[slot].enabled && slots_[slot].current <= 1.0f;
}

bool FaderSlotArray::is_complete(int slot) const {
    return valid(slot) && slots_[slot].enabled && slots_[slot].current == 1.0f;
}

void FaderSlotArray::set_enabled(int slot, bool enabled) {
    if (valid(slot)) slots_[slot].enabled = enabled;
}

void FaderSlotArray::set_duration(int slot, float seconds) {
    if (valid(slot)) slots_[slot].duration = seconds;
}

void FaderSlotArray::set_texture(int slot, void* texture_object) {
    if (valid(slot)) slots_[slot].texture_object = texture_object;
}

void FaderSlotArray::set_current(int slot, float current) {
    if (valid(slot)) slots_[slot].current = current;
}

std::string FaderSlotArray::texture_path(const std::string& name) {
    return std::string(kTextureDir) + name;
}
