// debug/MemTrace.h
#ifndef MEMTRACE_H
#define MEMTRACE_H
#include <string>
#include <vector>
#include <cstdint>

class TextRenderer;

namespace mem {

struct A { const char* file; int line; size_t bytes; };

void start();
void stop();
const std::vector<A>& allocs();
float frame_diff();
bool  growing();
void  tick();
void  draw(TextRenderer& tr, float x, float y);

} // namespace mem
#endif
