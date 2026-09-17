// debug/NavView.h
#ifndef NAVVIEW_H
#define NAVVIEW_H
#include <glm/glm.hpp>
#include <vector>

class TextRenderer;

namespace nv {

std::vector<glm::vec3> path();
std::vector<glm::vec3> zoneVerts();
std::vector<float>     zoneDist();
std::vector<glm::vec3> portals();
bool isPathing();

void raycast_path(float sx, float sy);
void set_agent(int npc, float rad, float spd);
void draw(TextRenderer& tr, float x, float y, float sx, float sy);

} // namespace nv
#endif
