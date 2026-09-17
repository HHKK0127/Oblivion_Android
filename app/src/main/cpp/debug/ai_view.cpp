// Phase 64 Debug: AI State Viewer Implementation

#include "ai_view.h"
#include "../game/ai_scheduler.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <vector>
#include <glm/glm.hpp>

namespace av {

static const char* typeName(int t) {
    switch (t) {
    case 0:  return "NONE";
    case 1:  return "EAT";
    case 2:  return "SLEEP";
    case 3:  return "WANDER";
    case 4:  return "TRAVEL";
    case 5:  return "PATROL";
    case 6:  return "FOLLOW";
    case 7:  return "GUARD";
    case 8:  return "COMBAT";
    case 9:  return "FLEE";
    case 10: return "ACCOMPANY";
    case 11: return "USE_ITEM";
    case 12: return "SAND_BOX";
    case 13: return "DIALOGUE";
    default: return "?";
    }
}

static std::vector<Row> rows(const ai::AIScheduler& sched, uint32_t npcId) {
    std::vector<Row> out;
    auto& stacks = sched.getPackageStacks();
    auto  it     = stacks.find(npcId);
    if (it == stacks.end()) return out;
    const auto& stack = it->second;

    const auto* act  = sched.getActivePackageForNpc(npcId);
    uint32_t  actId  = act ? act->packageId : 0xFFFFFFFF;

    for (const auto& pkg : stack.getPackages()) {
        Row r;
        r.pkgId   = pkg.packageId;
        r.type    = static_cast<int>(pkg.type);
        r.prio    = pkg.priority;
        r.active  = pkg.active;
        r.met     = pkg.conditionsMet;
        r.elapsed = pkg.elapsed;
        r.cur     = (pkg.packageId == actId);
        out.push_back(r);
    }
    return out;
}

void draw(TextRenderer& tr, const ai::AIScheduler& sched,
          uint32_t npcId, float x, float y) {
    glm::vec3 yellow = {1.f, 0.9f, 0.1f};
    glm::vec3 green  = {0.3f, 0.9f, 0.3f};
    glm::vec3 grey   = {0.6f, 0.6f, 0.6f};

    char b[96];
    snprintf(b, sizeof(b), "AI [%u]", npcId);
    tr.renderText(b, x, y, yellow, 0.9f);
    y -= 0.025f;

    for (const auto& r : rows(sched, npcId)) {
        glm::vec3 c = r.cur    ? yellow
                    : r.active ? green
                    :            grey;
        snprintf(b, sizeof(b), "%u %s p%d %s %.2fs",
                 r.pkgId, typeName(r.type), r.prio,
                 r.met ? "M" : " ", r.elapsed);
        tr.renderText(b, x, y, c, 0.8f);
        y -= 0.02f;
    }
}

} // namespace av
