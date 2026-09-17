#pragma once

// Phase 64 Debug: Animation Scrubber
// Display and scrub active animation sequences

#include <cstdint>

class TextRenderer;

namespace animation { class AnimationPlayer; }

namespace as {

struct Row {
    uint32_t seqIdx;
    float    cur;
    float    speed;
    bool     active;
    bool     looping;
};

void draw(TextRenderer& tr, const animation::AnimationPlayer& ap,
          float x, float y);

void scrub(animation::AnimationPlayer& ap, uint32_t seqIdx, float sec);
void step(animation::AnimationPlayer& ap, uint32_t seqIdx, int dir);

} // namespace as
