// debug/QuestDbg.h
#ifndef QUESTDBG_H
#define QUESTDBG_H
#include <glm/glm.hpp>
#include <string>
#include <vector>

class TextRenderer;

namespace qdb {

struct Q { std::string name; bool flag; };
std::vector<Q> flags();
void set_flag(const std::string& n, bool v);
int  cur_quest();

struct N { int node; std::string text; std::string cond; bool ok; };
std::vector<N> nodes(int questId);
void jump_node(int questId, int nodeId);

void draw(TextRenderer& tr, float x, float y, const char* search);

} // namespace qdb
#endif
