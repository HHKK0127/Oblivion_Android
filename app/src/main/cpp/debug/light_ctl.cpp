// debug/LightCtl.cpp
#include "light_ctl.h"
#include <cstring>
#include <glm/glm.hpp>

class TextRenderer;

namespace lct {

static glm::vec4 ambient_col{0.3f, 0.3f, 0.4f, 1.f};
static glm::vec4 dir_col{0.9f, 0.85f, 0.7f, 1.f};
static float fog_density = 0.01f;
static glm::vec3 fog_color{0.5f, 0.6f, 0.7f};
static float fog_start = 10.f, fog_end = 500.f;
static float water_height = 0.f, water_norm = 1.f;
static float sky_speed = 1.f;

void set_ambient(const glm::vec3& c, float i){ ambient_col = glm::vec4(c, i); }
void set_dir(const glm::vec3& c, float i){ dir_col = glm::vec4(c, i); }
glm::vec4 get_ambient(){ return ambient_col; }
glm::vec4 get_dir(){ return dir_col; }

void set_fog(float d,const glm::vec3&c,float s,float e){
    fog_density=d; fog_color=c; fog_start=s; fog_end=e;
}
void set_water(float h,float n){ water_height=h; water_norm=n; }
void set_sky_speed(float s){ sky_speed=s; }

void draw(TextRenderer& tr,float x,float y){
    char b[128];
    snprintf(b,sizeof(b),"ambient(%.2f,%.2f,%.2f) i=%.2f",
             ambient_col.r,ambient_col.g,ambient_col.b,ambient_col.a);
    tr.renderText(b,x,y,glm::vec3(0.9f,0.85f,0.7f),0.85f);
    snprintf(b,sizeof(b),"dir(%.2f,%.2f,%.2f) i=%.2f",
             dir_col.r,dir_col.g,dir_col.b,dir_col.a);
    tr.renderText(b,x,y-0.018f,glm::vec3(1.f,0.9f,0.6f),0.85f);
    snprintf(b,sizeof(b),"fog: d=%.3f s=%.0f e=%.0f",fog_density,fog_start,fog_end);
    tr.renderText(b,x,y-0.036f,glm::vec3(0.6f,0.7f,0.9f),0.8f);
    snprintf(b,sizeof(b),"water: h=%.1f n=%.2f",water_height,water_norm);
    tr.renderText(b,x,y-0.054f,glm::vec3(0.3f,0.6f,0.9f),0.8f);
    snprintf(b,sizeof(b),"sky speed=%.2fx",sky_speed);
    tr.renderText(b,x,y-0.072f,glm::vec3(0.5f,0.8f,1.f),0.8f);
}

} // namespace lct
