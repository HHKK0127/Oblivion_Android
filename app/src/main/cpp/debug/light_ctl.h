// debug/LightCtl.h
#ifndef LIGHTCTL_H
#define LIGHTCTL_H
#include <glm/glm.hpp>

class TextRenderer;

namespace lct {

void set_ambient(const glm::vec3& c, float inten);
void set_dir(const glm::vec3& c, float inten);
glm::vec4 get_ambient();
glm::vec4 get_dir();

void set_fog(float density, const glm::vec3& col, float start, float end);
void set_water(float height, float norm);
void set_sky_speed(float s);

void draw(TextRenderer& tr, float x, float y);

} // namespace lct
#endif
