// debug/HotReload.h
#ifndef HOTRELOAD_H
#define HOTRELOAD_H
#include <string>

namespace hr {

void watch_init(const char* assets_dir);
void poll();
bool changed_nif(const char* file);
bool changed_shdr(const char* file);
bool changed_pex(const char* file);
void reload_nif(const char* file);
void reload_shdr(const char* file);
void reload_pex(const char* file);
const char* last_event();

} // namespace hr
#endif
