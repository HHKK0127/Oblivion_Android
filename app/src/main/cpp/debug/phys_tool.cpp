// debug/PhysTool.cpp
#include "phys_tool.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <glm/glm.hpp>

class TextRenderer;

namespace phys {

static std::vector<glm::vec3> contact_list;

void apply_force(uint32_t n,const glm::vec3&d,float m){
    // world.force(n,d,m);
    (void)n; (void)d; (void)m;
}

void apply_impulse(uint32_t n,const glm::vec3&d,float m){
    // world.impulse(n,d,m);
    (void)n; (void)d; (void)m;
}

float get_mass(uint32_t n){ return 1.f; }
float get_damp(uint32_t n){ return 0.1f; }
void  set_mass(uint32_t n,float v){ (void)n; (void)v; }
void  set_damp(uint32_t n,float v){ (void)n; (void)v; }
std::vector<glm::vec3>& contacts(){ return contact_list; }

void draw(TextRenderer& tr,float x,float y,uint32_t npc){
    char b[96];
    snprintf(b,sizeof(b),"mass=%.2f damp=%.2f",get_mass(npc),get_damp(npc));
    tr.renderText(b,x,y,glm::vec3(0.8f,0.9f,0.2f),0.9f);
    snprintf(b,sizeof(b),"contacts=%zu",contacts().size());
    tr.renderText(b,x,y-0.018f,glm::vec3(0.9f,0.4f,0.2f),0.8f);
    tr.renderText("[drag]=apply [mass][damp] sliders",x,y-0.036f,glm::vec3(0.5f,0.5f,0.5f),0.8f);
}

} // namespace phys
