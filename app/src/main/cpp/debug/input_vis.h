// debug/InputVis.h
#ifndef INPUTVIS_H
#define INPUTVIS_H
#include <glm/glm.hpp>
#include <vector>

class TextRenderer;

namespace inv {

struct T { glm::vec2 pos, vel; bool active; };
std::vector<T>& touches();
glm::vec4 pad_axis();
glm::vec4 pad_btn();
void draw(TextRenderer& tr, float x, float y);

} // namespace inv
#endif
