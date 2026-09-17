// debug/ScriptDbg.h
#ifndef SCRIPTDBG_H
#define SCRIPTDBG_H
#include <glm/glm.hpp>
#include <string>
#include <vector>

class TextRenderer;

namespace sdb {

struct WP { int id; float val; }; // watch expression

// Breakpoint control
void set_bp(const std::string& script, int line);
void clr_bp();
bool bp_hit();

// VM access
float read_local(int idx);
void  write_local(int idx, float v);
float peek_stack(int idx);
const char* cur_script();
int     cur_line();

// Step execution
void run_step();
void run_next();
void run_cont();

// Display
void draw(TextRenderer& tr, float x, float y,
          const std::string& buf, int bufcap);

} // namespace sdb
#endif
