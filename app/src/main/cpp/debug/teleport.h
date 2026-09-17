#pragma once

// Phase 64 Debug: Teleport
// Teleport to preset locations and spawn/despawn NPCs

#include <cstdint>

class Camera;
class WorldLoader;

namespace tp {

struct Preset {
    char label[24];
    int32_t gx, gy;
    float x, y, z, yaw, pitch;
};

const Preset* presets();
int presetCount();

void go(int i, Camera& cam);
void spawn(uint32_t npcId, const Camera& cam, WorldLoader& loader);
void despawn(uint32_t npcId, WorldLoader& loader);

} // namespace tp
