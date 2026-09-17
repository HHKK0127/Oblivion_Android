// debug/SlotView.cpp
#include "slot_view.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <ctime>
#include <string>
#include <glm/glm.hpp>

class TextRenderer;
class WorldLoader;
class Camera;

namespace sv {

static std::string action;

std::vector<Slot> slots() {
    std::vector<Slot> out;
    for (int i = 0; i < 10; ++i) {
        Slot s; s.index = i; s.level = 0; s.pname = "empty";
        out.push_back(s);
    }
    return out;
}

void load (int slot, WorldLoader& wl, Camera& cam) {
    action = "LOAD slot=" + std::to_string(slot);
}
void copy (int f, int t) { action = "COPY "+std::to_string(f)+"->"+std::to_string(t); }
void erase(int s)         { action = "ERASE slot="+std::to_string(s); }
const char* last_action() { return action.c_str(); }

Diff compare(int a, int b) {
    Diff d;
    d.level_diff = 0;
    d.pos_diff = glm::vec3(0.f, 0.f, 0.f);
    d.scene_a = "scene_a";
    d.scene_b = "scene_b";
    return d;
}

void draw (TextRenderer& tr, float x, float y,
           int sel, int cmp, const char* input) {
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "%s", last_action());
    tr.renderText(tmp, x, y, glm::vec3(0.9f,0.8f,0.2f), 0.9f);

    float yy = y - 0.02f;
    for (const auto& s : slots()) {
        glm::vec3 c = (s.index == sel) ? glm::vec3(1.f,0.9f,0.1f)
                    : (s.index == cmp) ? glm::vec3(0.2f,0.8f,1.f)
                                       : glm::vec3(0.6f,0.6f,0.6f);
        char b[160];
        snprintf(b,sizeof(b),"[%d] %s L%d %s",
                 s.index, s.date.c_str(), s.level, s.scene.c_str());
        tr.renderText(b, x, yy, c, 0.8f);
        snprintf(b,sizeof(b),"   %s %.1f/%.1f/%.1f",
                 s.pname.c_str(), s.pos.x, s.pos.y, s.pos.z);
        tr.renderText(b, x, yy, glm::vec3(0.5f,0.5f,0.5f), 0.7f);
        yy -= 0.036f;
    }

    if (cmp >= 0) {
        Diff d = compare(sel, cmp);
        char b[128];
        snprintf(b,sizeof(b),"CMP: dL=%d [%s]->[%s]",
                 d.level_diff, d.scene_a.c_str(), d.scene_b.c_str());
        tr.renderText(b, x, yy, glm::vec3(0.2f,0.9f,0.9f), 0.85f);
        yy -= 0.02f;
    }

    if (input && input[0]) {
        snprintf(tmp,sizeof(tmp),"desc: %s", input);
        tr.renderText(tmp, x, yy, glm::vec3(0.5f,0.9f,0.6f), 0.8f);
    }
    tr.renderText("[tap]=load [hold]=del [CMP]=compare",
                  x, yy - 0.02f, glm::vec3(0.4f,0.4f,0.4f), 0.7f);
}

} // namespace sv
