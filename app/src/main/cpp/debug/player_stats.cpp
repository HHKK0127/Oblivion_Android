// debug/PlayerStats.cpp
#include "player_stats.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <cstdio>
#include <glm/glm.hpp>

class TextRenderer;

namespace ps {

static Stat stats[]={
    {"str",10.f,10.f,0,"attr"},
    {"int",10.f,10.f,0,"attr"},
    {"wil",10.f,10.f,0,"attr"},
    {"agi",10.f,10.f,0,"attr"},
    {"spd",10.f,10.f,0,"attr"},
    {"end",10.f,10.f,0,"attr"},
    {"per",10.f,10.f,0,"attr"},
    {"luk",10.f,10.f,0,"attr"},
    {"blade",25.f,25.f,0,"skill"},
    {"blunt",20.f,20.f,0,"skill"},
    {"h2h",  15.f,15.f,0,"skill"},
    {"block",20.f,20.f,0,"skill"},
};

void draw(TextRenderer& tr, float x, float y){
    char b[96];
    float yy=y;
    for (auto& s : stats){
        int neg=(s.mod<0);
        int pct=(int)((s.val/s.base)*100.f);
        snprintf(b,sizeof(b),"%s %c%.0f/%.0f (%+d) [%d%%]",
                 s.label, neg?'-':' ', s.val,s.base,s.mod,pct);
        glm::vec3 c=(pct>=120)?glm::vec3(0.2f,0.9f,0.2f):
                    (pct<=80 )?glm::vec3(0.9f,0.2f,0.2f):
                                glm::vec3(0.7f,0.7f,0.7f);
        tr.renderText(b,x,yy,c,0.8f);
        yy-=0.015f;
    }
    tr.renderText("STR..LUC  |  blade..block",
                  x,yy-0.01f,glm::vec3(0.4f,0.4f,0.4f),0.7f);
}

} // namespace ps
