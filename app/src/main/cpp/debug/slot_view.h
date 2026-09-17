// debug/SlotView.h
#ifndef SLOTVIEW_H
#define SLOTVIEW_H
#include <glm/glm.hpp>
#include <string>
#include <vector>

class TextRenderer;
class WorldLoader;
class Camera;

namespace sv {

struct Slot {
    int         index;
    std::string date;
    std::string pname;
    int         level;
    glm::vec3   pos;
    std::string scene;
    std::string desc;
};

std::vector<Slot> slots();
void  load (int slot, WorldLoader& wl, Camera& cam);
void  copy (int from, int to);
void  erase(int slot);
const char* last_action();

struct Diff {
    int level_diff;
    glm::vec3 pos_diff;
    std::string scene_a, scene_b;
};
Diff compare(int a, int b);

void  draw (TextRenderer& tr, float x, float y,
            int sel, int cmp, const char* input);

} // namespace sv
#endif
