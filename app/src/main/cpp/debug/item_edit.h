// debug/ItemEdit.h
#ifndef ITEMEDIT_H
#define ITEMEDIT_H
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

class TextRenderer;

namespace ite {

struct I { uint32_t id; int stack; int ench; };
std::vector<I> list();
void add(uint32_t id, int stack);
void bump(uint32_t id, int delta);
void remove(uint32_t id);
void draw(TextRenderer& tr, float x, float y, const char* input);

} // namespace ite
#endif
