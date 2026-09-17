// debug/DialogDbg.h
#ifndef DIALOGDBG_H
#define DIALOGDBG_H
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

class TextRenderer;

namespace dbg {

void begin(uint32_t npcId, int topic);
struct N { int node; std::string text; std::string cond; bool ok; };
std::vector<N> tree();
bool node_eval(const char* cond);
void jump(int nodeId);
void draw(TextRenderer& tr, float x, float y, uint32_t npc);

} // namespace dbg
#endif
