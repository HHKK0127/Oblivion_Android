// debug/AudioBrowse.cpp
#include "audio_browse.h"
#include "../ui/text_renderer.h"
#include <cstring>
#include <cmath>
#include <glm/glm.hpp>

class TextRenderer;

namespace ab {

std::vector<F> list(const char* dir, const char* ext){
    std::vector<F> out;
    // DIR* d = opendir(dir);
    // if (!d) return out;
    // struct dirent* e;
    // while ((e = readdir(d))) {
    //     std::string n = e->d_name;
    //     if (strstr(n.c_str(),ext)) out.push_back({n});
    // }
    // closedir(d);
    (void)dir; (void)ext;
    return out;
}

void play(const char* n){
    // AudioManager::playSE(audio_id(n));
    (void)n;
}

const float* wave(const char* n,int& p){
    static float dummy[256]={0};
    p = 256;
    return dummy;
}

float gain(const char* n){
    return 1.0f; // audio_gain(n)
}

void draw(TextRenderer& tr,float x,float y,const char* sel,const char* filter){
    float yy=y;
    char tmp[128];
    for (auto& f : list("audio", filter)){
        if (filter && !strstr(f.name.c_str(),filter)) continue;
        snprintf(tmp,sizeof(tmp),"[%s] %.1f", f.name.c_str(), gain(f.name.c_str()));
        tr.renderText(tmp,x,yy,
            (sel && strcmp(f.name.c_str(),sel)==0)?glm::vec3(1.f,1.f,0.2f):glm::vec3(0.6f,0.9f,0.6f),0.8f);
        yy-=0.018f;
    }
    tr.renderText("[tap]=play",x,yy,glm::vec3(0.5f,0.5f,0.5f),0.8f);
}

} // namespace ab
