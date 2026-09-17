// debug/PlayerStats.h
#ifndef PLAYERSTATS_H
#define PLAYERSTATS_H
#include <vector>
#include <string>

class TextRenderer;

namespace ps {

void draw(TextRenderer& tr, float x, float y);

struct Stat {
    const char* label;
    float val;
    float base;
    int   mod;
    const char* cat;
};

} // namespace ps
#endif
