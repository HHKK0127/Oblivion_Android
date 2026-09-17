#pragma once

// Phase 64 Debug: NPC Picker
// Tap-to-select NPC via raycast, show info HUD

#include <cstdint>

class Camera;
class CollisionWorld;
class WorldLoader;
class TextRenderer;

namespace pk {

void set_armed(bool a);
bool has_pick();
uint32_t id();

void tap(const Camera& cam, CollisionWorld& coll,
         WorldLoader& loader, float sx, float sy);

void teleport(Camera& cam, WorldLoader& loader);

void info(TextRenderer& tr, float x, float y);

} // namespace pk
