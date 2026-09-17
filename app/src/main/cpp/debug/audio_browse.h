// debug/AudioBrowse.h
#ifndef AUDIOBROWSE_H
#define AUDIOBROWSE_H
#include <glm/glm.hpp>
#include <string>
#include <vector>

class TextRenderer;

namespace ab {

struct F { std::string name; };
std::vector<F> list(const char* dir, const char* ext=".wav");
void  play(const char* name);
const float* wave(const char* name, int& pts);
float gain(const char* name);
void draw(TextRenderer& tr, float x, float y, const char* sel, const char* filter);

} // namespace ab
#endif
