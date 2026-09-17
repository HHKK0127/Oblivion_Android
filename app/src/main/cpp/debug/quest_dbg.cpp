// debug/QuestDbg.cpp
#include "quest_dbg.h"
#include <cstring>
#include <glm/glm.hpp>

class TextRenderer;

namespace qdb {

std::vector<Q> flags() {
    std::vector<Q> out;
    // for (auto& f : getQuests().all()) {
    //     out.push_back({f.name, f.flag});
    // }
    return out;
}

void set_flag(const std::string& n, bool v) {
    // getQuests().set(n, v);
}

int cur_quest() {
    return 0; // getQuests().active();
}

std::vector<N> nodes(int qid) {
    std::vector<N> out;
    // for (auto& n : getQuests().tree(qid)) {
    //     out.push_back({n.id, n.text, n.cond, n.evaluated});
    // }
    return out;
}

void jump_node(int qid, int nid) {
    // getQuests().goto_node(qid, nid);
}

void draw(TextRenderer& tr, float x, float y, const char* search) {
    char tmp[128];
    snprintf(tmp,sizeof(tmp),"active=%d",cur_quest());
    tr.renderText(tmp,x,y,glm::vec3(0.9f,0.8f,0.2f),0.9f);
    float yy=y-0.02f;
    for (auto& f : flags()) {
        if (search && !strstr(f.name.c_str(),search)) continue;
        snprintf(tmp,sizeof(tmp),"%c %s", f.flag?'1':'0', f.name.c_str());
        tr.renderText(tmp,x,yy, f.flag?glm::vec3(0.3f,0.9f,0.4f):glm::vec3(0.5f,0.5f,0.5f),0.8f);
        yy -= 0.018f;
    }
}

} // namespace qdb
