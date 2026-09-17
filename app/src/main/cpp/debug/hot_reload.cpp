// debug/HotReload.cpp
#include "hot_reload.h"
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef __ANDROID__
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace hr {

static std::string event;

#ifdef __ANDROID__
static int ifd = -1;
static std::unordered_map<uint32_t,std::string> watchers;
#endif

void watch_init(const char* dir) {
#ifdef __ANDROID__
    ifd = inotify_init1(IN_NONBLOCK);
    if (ifd < 0) return;
    uint32_t wd = inotify_add_watch(ifd, dir, IN_MODIFY|IN_CLOSE_WRITE);
    if (wd != (uint32_t)-1) watchers[wd] = dir;
#else
    (void)dir;
#endif
}

void poll() {
#ifdef __ANDROID__
    if (ifd < 0) return;
    char buf[4096] __attribute__((aligned(8)));
    ssize_t len = read(ifd, buf, sizeof(buf));
    if (len <= 0) return;
    for (char* ptr = buf; ptr < buf + len; ) {
        struct inotify_event* ev = (struct inotify_event*)ptr;
        auto it = watchers.find(ev->wd);
        if (it != watchers.end()) {
            std::string fname = it->second + "/" + ev->name;
            if (fname.size() > 4) {
                std::string ext = fname.substr(fname.rfind('.'));
                if (ext == ".nif")       reload_nif(fname.c_str());
                else if (ext == ".glsl") reload_shdr(fname.c_str());
                else if (ext == ".pex")  reload_pex(fname.c_str());
            }
        }
        ptr += sizeof(struct inotify_event) + ev->len;
    }
#endif
}

bool changed_nif(const char*)  { return strstr(last_event(),".nif")!=nullptr; }
bool changed_shdr(const char*) { return strstr(last_event(),".glsl")!=nullptr; }
bool changed_pex(const char*)  { return strstr(last_event(),".pex")!=nullptr; }
const char* last_event() { return event.c_str(); }

void reload_nif(const char*f){
    event=f;
    // meshloader_reload(f);
}
void reload_shdr(const char*f){
    event=f;
    // shader_recompile(f);
}
void reload_pex(const char*f){
    event=f;
    // vm_bytecode_load(f);
}

} // namespace hr
