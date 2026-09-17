// debug/NavView.cpp
#include "nav_view.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <glm/glm.hpp>

class TextRenderer;

namespace nv {

static std::vector<glm::vec3> s_path, s_zones, s_portals;
static std::vector<float>     s_dist;
static bool s_pathing = false;
static int  s_agent_npc = 0;
static float s_agent_rad = 0.3f, s_agent_spd = 3.5f;

std::vector<glm::vec3> path()     { return s_path; }
std::vector<glm::vec3> zoneVerts(){ return s_zones; }
std::vector<float>     zoneDist() { return s_dist; }
std::vector<glm::vec3> portals()  { return s_portals; }
bool isPathing(){ return s_pathing; }

void set_agent(int npc, float rad, float spd){
    s_agent_npc = npc; s_agent_rad = rad; s_agent_spd = spd;
}

void raycast_path(float sx, float sy) {
    s_path.clear();
    s_pathing = true;
    // pathfinder.path(eye, target)
    (void)sx; (void)sy;
}

void draw(TextRenderer& tr, float x, float y, float sx, float sy){
    char b[128];
    snprintf(b,sizeof(b),"zones=%zu paths=%zu portals=%zu",
             s_zones.size(), s_path.size(), s_portals.size());
    tr.renderText(b,x,y,glm::vec3(0.4f,0.9f,0.6f),0.9f);
    float yy=y-0.018f;
    for (size_t i=0;i<std::min((size_t)5,s_zones.size());++i){
        snprintf(b,sizeof(b),"zone[%zu] d=%.1f",i,s_dist[i]);
        tr.renderText(b,x,yy,glm::vec3(0.5f,0.8f,0.9f),0.8f);
        yy-=0.015f;
    }
    snprintf(b,sizeof(b),"agent[%d] r=%.2f s=%.1f",s_agent_npc,s_agent_rad,s_agent_spd);
    tr.renderText(b,x,yy,glm::vec3(0.9f,0.8f,0.2f),0.85f);
    yy-=0.015f;
    snprintf(b,sizeof(b),"tap=(%.2f,%.2f) path=%s",sx,sy,s_pathing?"yes":"no");
    tr.renderText(b,x,yy,glm::vec3(0.5f,0.5f,0.5f),0.8f);
}

} // namespace nv
