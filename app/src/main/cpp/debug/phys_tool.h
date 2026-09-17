// debug/PhysTool.h
#ifndef PHYSICSTOOL_H
#define PHYSICSTOOL_H
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class TextRenderer;

namespace phys {

void apply_force(uint32_t npc, const glm::vec3& dir, float mag);
void apply_impulse(uint32_t npc, const glm::vec3& dir, float mag);
float get_mass(uint32_t npc);
float get_damp(uint32_t npc);
void  set_mass(uint32_t npc, float v);
void  set_damp(uint32_t npc, float v);
std::vector<glm::vec3>& contacts();
void draw(TextRenderer& tr, float x, float y, uint32_t npc);

} // namespace phys
#endif
