// debug/ScriptDbg.cpp
#include "script_dbg.h"
#include <cstring>
#include <unordered_set>
#include <glm/glm.hpp>

class TextRenderer;

namespace sdb {

static std::unordered_set<std::string> bps;
static bool cont = false;

void set_bp(const std::string& script, int) { bps.insert(script); }
void clr_bp()                               { bps.clear(); }
bool bp_hit()                               { return bps.count(cur_script()) != 0; }

float read_local(int i)   { return 0.f; } // <- VM_local(i)
void  write_local(int i,float v){}         // <- VM_setlocal(i,v)
float peek_stack(int i)   { return 0.f; } // <- VM_stack(i)
const char* cur_script()  { return "-"; }  // <- VM_script()
int     cur_line()        { return 0; }    // <- VM_line()

void run_step() {
    // vm_step();
    if (bps.count(cur_script())) cont = false;
}
void run_next() {
    // do { vm_step(); } while (!bps.count(VM_script()) && VM_same_script());
}
void run_cont() {
    cont = true;
    // while (!bps.count(VM_script()) && VM_active()) vm_step();
}

void draw(TextRenderer& tr, float x, float y,
          const std::string& buf, int) {
    char b[160];
    snprintf(b,sizeof(b),"scr=%s line=%d", cur_script()?cur_script():"-", cur_line());
    tr.renderText(b, x, y, glm::vec3(0.9f,0.9f,0.2f), 0.9f);
    snprintf(b,sizeof(b),"bp=%s  mode=%s",
             cont?"CONT":"HALT", cont?"run":"halt");
    tr.renderText(b, x, y+0.02f, cont?glm::vec3(0.3f,0.9f,0.4f):glm::vec3(0.9f,0.2f,0.2f),0.9f);
    for (int i=0;i<4;i++){
        snprintf(b,sizeof(b),"sp%d=%.4f",i,peek_stack(i));
        tr.renderText(b, x, y+0.04f+i*0.018f, glm::vec3(0.6f,0.9f,1.f),0.8f);
    }
    if (!buf.empty()) snprintf(b,sizeof(b),"watch=%s",buf.c_str());
    else snprintf(b,sizeof(b),"watch input:");
    tr.renderText(b, x, y-0.02f, glm::vec3(0.7f,0.7f,0.7f),0.8f);
}

} // namespace sdb
