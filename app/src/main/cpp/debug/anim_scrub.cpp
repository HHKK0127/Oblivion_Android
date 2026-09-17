// Phase 64 Debug: Animation Scrubber Implementation

#include "anim_scrub.h"
#include "../animation/animation_player.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <vector>
#include <glm/glm.hpp>

namespace as {

static std::vector<Row> rows(const animation::AnimationPlayer& ap) {
    std::vector<Row> out;
    const auto& seqs = ap.getActiveSequences();
    for (const auto& s : seqs) {
        Row r;
        r.seqIdx  = s.sequenceIndex;
        r.cur     = s.currentTime;
        r.speed   = s.playbackSpeed;
        r.active  = s.active;
        r.looping = s.looping;
        out.push_back(r);
    }
    return out;
}

void scrub(animation::AnimationPlayer& ap, uint32_t seqIdx, float sec) {
    ap.seekTo(seqIdx, sec);
}

void step(animation::AnimationPlayer& ap, uint32_t seqIdx, int dir) {
    ap.stepFrame(seqIdx, dir);
}

void draw(TextRenderer& tr, const animation::AnimationPlayer& ap,
          float x, float y) {
    constexpr float BAR_MAX = 10.f;
    glm::vec3 green = {0.2f, 0.9f, 0.4f};
    glm::vec3 grey  = {0.5f, 0.5f, 0.5f};

    char b[96];
    snprintf(b, sizeof(b), "ANIM");
    tr.renderText(b, x, y, green, 0.9f);
    y -= 0.025f;

    for (const auto& r : rows(ap)) {
        glm::vec3 c = r.active ? green : grey;
        snprintf(b, sizeof(b), "[%u] %.3fs x%.1f %s%s",
                 r.seqIdx, r.cur, r.speed,
                 r.active ? "*" : " ",
                 r.looping ? "L" : "");
        tr.renderText(b, x, y, c, 0.8f);
        y -= 0.02f;
    }
}

} // namespace as
