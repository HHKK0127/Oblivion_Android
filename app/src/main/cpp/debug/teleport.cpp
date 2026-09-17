// Phase 64 Debug: Teleport Implementation

#include "teleport.h"
#include "../world/world_manager.h"
#include "../world/world_entity.h"
#include "../engine/camera.h"
#include <cstring>

namespace tp {

static const Preset P[10] = {
    {"city",     0,  0,   100,15, 200, 1.57f,-0.1f},
    {"castle",  -1,  0,  -150,30, -80, 0.78f, 0.0f},
    {"cave",    -1, -1,   -50, -2,-80, 3.14f, 0.05f},
    {"forest",   1, -1,   500,20,-300, 0.78f,-0.05f},
    {"temple",   0, -1,    20,10,  50, 0.0f,  0.0f},
    {"swamp",   -2,  0,  -200, 5,-150, 1.57f, 0.0f},
    {"waste",    2, -2,   800,15,-500, 0.78f,-0.02f},
    {"aqua",    -1,  1,   -30, -5, 20, 1.57f, 0.0f},
    {"bridge",   0,  1,    60,40,  90, 0.78f,-0.1f},
    {"throne",   0,  0,    10,20, -10, 3.14f, 0.0f},
};

const Preset* presets() { return P; }
int presetCount() { return 10; }

void go(int i, Camera& cam) {
    if (i < 0 || i >= 10) return;
    const auto& p = P[i];
    // WorldManager::loadCell(int32_t, int32_t) - use existing global instance
    cam.setPosition(glm::vec3(p.x, p.y, p.z));
    cam.rotate(p.yaw, p.pitch);
}

void spawn(uint32_t npcId, const Camera& cam, WorldLoader& loader) {
    glm::vec3 fwd = cam.getForward();
    glm::vec3 tgt = cam.getPosition() + fwd * 10.f;
    // loadActor requires nifPath - npcId lookup needed
    // Placeholder: caller should resolve npcId to nifPath first
    (void)npcId;
    (void)loader;
}

void despawn(uint32_t npcId, WorldLoader& loader) {
    auto* e = loader.getEntityByNpcId(npcId);
    if (e) {
        loader.unload(*e);
    }
}

} // namespace tp
