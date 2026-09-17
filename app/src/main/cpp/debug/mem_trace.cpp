// debug/MemTrace.cpp
#include "mem_trace.h"
#include <cstring>
#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>

class TextRenderer;

namespace mem {

static std::vector<A> live;
static float  prev_kb=0.f, cur_kb=0.f;
static int    grow_streak=0;
static bool   enabled=false;

void start() { enabled = true; }
void stop()  { enabled = false; }

void tick(){
    cur_kb = 0.f;
    for (auto& a : live) cur_kb += (float)a.bytes / 1024.f;
    if (cur_kb > prev_kb + 1.f) grow_streak++; else grow_streak = 0;
    prev_kb = cur_kb;
}

float frame_diff(){ return cur_kb - prev_kb; }
bool  growing(){ return grow_streak > 30; }
const std::vector<A>& allocs(){ return live; }

void draw(TextRenderer& tr,float x,float y){
    char b[96];
    snprintf(b,sizeof(b),"heap=%.1fKB diff=%+.1fKB",cur_kb,frame_diff());
    tr.renderText(b,x,y,growing()?glm::vec3(0.9f,0.2f,0.2f):glm::vec3(0.5f,0.9f,0.5f),0.9f);
    if (growing()) tr.renderText("[!] possible leak",x,y+0.02f,glm::vec3(1.f,0.3f,0.3f),0.8f);
    float yy=y-0.02f;
    for (size_t i=0;i<live.size()&&i<10;i++){
        snprintf(b,sizeof(b),"%s:%d %.0fB",live[i].file,live[i].line,(float)live[i].bytes);
        tr.renderText(b,x,yy,glm::vec3(0.6f),0.7f);
        yy-=0.016f;
    }
    tr.renderText(enabled?"[ON]":"[OFF]",x,y+0.04f,enabled?glm::vec3(0.3f,0.9f,0.4f):glm::vec3(0.9f,0.3f,0.3f),0.8f);
}

} // namespace mem
