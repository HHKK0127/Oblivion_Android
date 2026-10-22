#pragma once

// Parsing and formatting for plugins.txt, the load-order file the retail
// Oblivion launcher writes next to the game data.
//
// File grammar (tolerant, because several tools write this file):
//   - one plugin filename per line
//   - a line whose first non-blank character is '#' is a comment
//   - a leading '*' marks the plugin as active; the marker is stripped
//   - leading/trailing blanks, CR and a UTF-8 BOM are ignored
//   - blank lines are skipped
//   - the first occurrence of a filename wins; later duplicates are dropped
//
// The '*' marker is only meaningful when the file uses it. A plain Oblivion
// plugins.txt lists active plugins without any marker, so a file with no
// marked line is read as "every listed plugin is active".

#include <string>
#include <vector>

namespace oblivion {

struct PluginListEntry {
    std::string name;    // filename with the marker and surrounding blanks removed
    bool active = true;  // false only when the file marks lines and this one is unmarked
};

// Parses the body of a plugins.txt file. Returns the entries in file order.
std::vector<PluginListEntry> parsePluginList(const std::string& contents);

// Serialises entries back to plugins.txt form, one name per line, prefixing
// active entries with '*'. Ends with a trailing newline; empty input yields "".
std::string serializePluginList(const std::vector<PluginListEntry>& entries);

// Names of the active entries, in load order.
std::vector<std::string> activePluginNames(const std::vector<PluginListEntry>& entries);

// Retail load order: Oblivion.esm first, then the remaining masters (.esm) and
// finally the plugins (.esp), each group compared case-insensitively by name.
// The sort is stable so entries that tie keep the order they were given in.
void sortPluginLoadOrder(std::vector<PluginListEntry>& entries);

// True when the name ends in ".esm" (case-insensitive).
bool isMasterFileName(const std::string& name);

}  // namespace oblivion
