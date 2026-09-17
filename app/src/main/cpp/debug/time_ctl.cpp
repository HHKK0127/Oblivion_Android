// debug/TimeCtl.cpp
#include "time_ctl.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <cstdio>
#include <glm/glm.hpp>

class TextRenderer;

namespace tc {

static float  s_scale  = 1.f;
static float  s_bias   = 0.f;
static float  s_minutes= 720.f; // noon
static float  s_alt    = 45.f;
static float  s_azi    = 180.f;
static int    s_weather= 0;
static const char* W_NAMES[]={
    "Clear","Cloudy","Rain","Storm","Fog","Snow","Overcast","Thunder"
};

float& time_scale(){ return s_scale; }
float& bias()      { return s_bias; }
float& minutes()   { return s_minutes; }
float  sun_alt()   { return s_alt; }
float  sun_azi()   { return s_azi; }
void   set_weather(int id){ s_weather = id; }
int    weather(){ return s_weather; }

void draw(TextRenderer& tr, float x, float y,
          float hour, float b, float alt, float azi, int w)
{
    char buf[128];
    snprintf(buf,sizeof(buf),"scale=%.2f bias=%.1f",hour,b);
    tr.renderText(buf,x,y,glm::vec3(0.9f,0.8f,0.3f),0.9f);
    snprintf(buf,sizeof(buf),"sun alt=%.1f azi=%.1f",alt,azi);
    tr.renderText(buf,x,y-0.018f,glm::vec3(0.9f,0.7f,0.2f),0.85f);
    int wi=(w<0||w>7)?0:w;
    snprintf(buf,sizeof(buf),"weather[%d]=%s",wi,W_NAMES[wi]);
    tr.renderText(buf,x,y-0.036f,glm::vec3(0.4f,0.8f,0.9f),0.85f);
    tr.renderText("[bias][scale] sliders",
                  x,y-0.054f,glm::vec3(0.5f,0.5f,0.5f),0.8f);
}

} // namespace tc
