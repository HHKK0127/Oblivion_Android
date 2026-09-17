// debug/ItemEdit.cpp
#include "item_edit.h"
#include <cstring>
#include <glm/glm.hpp>

class TextRenderer;

namespace ite {

std::vector<I> list() {
    std::vector<I> out;
    // for (auto& it : getPlayer().inventory()) {
    //     out.push_back({it.itemId, it.stack, it.enchant});
    // }
    return out;
}

void add (uint32_t id,int s){ /* getPlayer().inv_add(id,s); */ }
void bump(uint32_t id,int d){ /* getPlayer().inv_bump(id,d); */ }
void remove(uint32_t id){ /* getPlayer().inv_del(id); */ }

void draw(TextRenderer& tr, float x, float y, const char* input) {
    float yy=y;
    for (auto& it : list()) {
        char b[80]; snprintf(b,sizeof(b),"%u x%d e%d",it.id,it.stack,it.ench);
        tr.renderText(b,x,yy,glm::vec3(0.8f,0.7f,0.9f),0.8f);
        yy -= 0.018f;
    }
    if (input && input[0]){
        char b[40]; snprintf(b,sizeof(b),"add id=%s",input);
        tr.renderText(b,x,yy,glm::vec3(0.5f,0.9f,0.6f),0.8f);
    }
}

} // namespace ite
