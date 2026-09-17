#pragma once

// Phase 64 Debug: AI State Viewer
// Display NPC AI package stack with current state highlight

#include <cstdint>

class TextRenderer;

namespace ai { class AIScheduler; }

namespace av {

struct Row {
    uint32_t pkgId;
    int      type;
    uint8_t  prio;
    bool     active;
    bool     met;
    float    elapsed;
    bool     cur;
};

void draw(TextRenderer& tr, const ai::AIScheduler& sched,
          uint32_t npcId, float x, float y);

} // namespace av
