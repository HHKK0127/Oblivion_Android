// debug/InputVis.cpp
#include "input_vis.h"
#include <cstring>
#include <cmath>
#include <glm/glm.hpp>

class TextRenderer;

namespace inv {

static std::vector<T> touch_list;

std::vector<T>& touches(){ return touch_list; }
glm::vec4 pad_axis(){ return glm::vec4(0.f); }
glm::vec4 pad_btn(){ return glm::vec4(0.f); }

void draw(TextRenderer& tr, float x, float y){
    for (auto& t : touches()){
        char b[32]; snprintf(b,sizeof(b),"v=%.1f",std::sqrt(t.vel.x*t.vel.x+t.vel.y*t.vel.y));
        tr.renderText(b,x + t.pos.x, y + t.pos.y, glm::vec3(0.2f,0.9f,1.f),0.7f);
    }
    glm::vec4 a=pad_axis(), bb=pad_btn();
    char b[64]; snprintf(b,sizeof(b),"axis(%.2f,%.2f) btn(%.0f,%.0f,%.0f,%.0f)",
        a.x,a.y,bb.x,bb.y,bb.z,bb.w);
    tr.renderText(b,x,y,glm::vec3(0.7f,0.5f,1.f),0.8f);
}

} // namespace inv
