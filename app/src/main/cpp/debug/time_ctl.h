// debug/TimeCtl.h
#ifndef TIMECTL_H
#define TIMECTL_H
#include <glm/glm.hpp>

class TextRenderer;

namespace tc {

float& time_scale();
float& bias();
float& minutes();
float  sun_alt();
float  sun_azi();
void   set_weather(int id);
int    weather();

void draw(TextRenderer& tr, float x, float y,
          float hour, float bias, float alt, float azi, int w);

} // namespace tc
#endif
