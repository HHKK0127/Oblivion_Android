// debug/DialogDbg.cpp
#include "dialog_dbg.h"
#include <cstring>
#include <glm/glm.hpp>

class TextRenderer;

namespace dbg {

void begin(uint32_t npc,int t){
    // getDialog().force_begin(npc,t);
}

std::vector<N> tree(){
    std::vector<N> out;
    // for (auto& n : getDialog().nodes()) {
    //     out.push_back({n.id, n.text, n.cond, n.evaluated});
    // }
    return out;
}

bool node_eval(const char* c){
    return false; // eval_cond(c);
}

void jump(int id){
    // getDialog().goto(id);
}

void draw(TextRenderer& tr,float x,float y,uint32_t npc){
    char tmp[64];
    snprintf(tmp,sizeof(tmp),"npc=%u",npc);
    tr.renderText(tmp,x,y,glm::vec3(0.1f,0.85f,0.95f),0.9f);
    float yy=y-0.02f;
    for (auto& n : tree()){
        char b[128]; snprintf(b,sizeof(b),"%d %s [%c]",n.node,n.text.c_str(),n.ok?'1':'0');
        tr.renderText(b,x,yy,n.ok?glm::vec3(0.3f,0.9f,0.4f):glm::vec3(0.9f,0.4f,0.2f),0.8f);
        yy-=0.018f;
    }
    tr.renderText("[node]=jump",x,yy,glm::vec3(0.5f,0.5f,0.5f),0.8f);
}

} // namespace dbg
