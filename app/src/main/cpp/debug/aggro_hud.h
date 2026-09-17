// debug/AggroHUD.h
#ifndef AGGROHUD_H
#define AGGROHUD_H
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>

class TextRenderer;

namespace ag {

struct Aggro {
    uint32_t npc;
    std::string name;
    float pct;
    glm::vec3 pos;
    float dist;
    int threat;
};

std::vector<Aggro> list();
void set_target(int npc);
int  target();
std::vector<uint32_t> enemy_list();
void draw(TextRenderer& tr, float x, float y, int tgt);

} // namespace ag
#endif
