// debug/AggroHUD.cpp
#include "aggro_hud.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>

class TextRenderer;

namespace ag {

static int s_target = -1;
static std::vector<Aggro> s_list;
static std::vector<uint32_t> s_enemies;

std::vector<Aggro> list(){ return s_list; }
void set_target(int npc){ s_target = npc; }
int  target(){ return s_target; }
std::vector<uint32_t> enemy_list(){ return s_enemies; }

void draw(TextRenderer& tr, float x, float y, int tgt){
    if (tgt < 0) {
        tr.renderText("target=n/a", x, y, glm::vec3(0.5f,0.5f,0.5f), 0.9f);
        return;
    }
    char b[128];
    snprintf(b,sizeof(b),"target=%d enemies=%zu",tgt,s_enemies.size());
    tr.renderText(b,x,y,glm::vec3(0.9f,0.3f,0.2f),0.9f);
    float yy=y-0.02f;
    for (auto& a : s_list){
        snprintf(b,sizeof(b),"[%u] %s %.0f%% dist=%.1f thr=%d",
                 a.npc,a.name.c_str(),a.pct,a.dist,a.threat);
        glm::vec3 c = (a.threat>=3)?glm::vec3(1.f,0.2f,0.2f):
                      (a.threat>=2)?glm::vec3(1.f,0.6f,0.1f):
                                    glm::vec3(0.8f,0.8f,0.8f);
        tr.renderText(b,x,yy,c,0.8f);
        yy-=0.015f;
    }
    tr.renderText("[tap]=set target",
                  x,yy-0.01f,glm::vec3(0.5f,0.5f,0.5f),0.7f);
}

} // namespace ag
