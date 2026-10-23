#pragma once

// Translation of TES IV FormIDs from a plugin's own index space into the
// engine's load-order space.
//
// Bits 24-31 of a FormID are an index, but which index depends on where the
// FormID is stored:
//
//   in a plugin file (on disk)  0 .. masters.size()-1  -> that entry of the
//                                                       plugin's own MAST list
//                               masters.size()         -> the plugin itself
//
//   in memory (what the engine   0 .. fileCount-1      -> position of the file
//    needs for lookups)                                 in load order
//
// Oblivion.esm has no masters, so its own index is 0 and its FormIDs are
// already in load-order space. Every other plugin has to be translated once,
// when it is added to the manager and before its records are indexed.
//
// The distinction matters because the two spaces differ: a plugin whose MAST
// list is [Oblivion.esm] loaded as the second file has master 0 (Oblivion.esm)
// at load-order index 0 and itself at load-order index 1. Rebasing every
// FormID onto the plugin's own index would move references to master records
// into the plugin's index, where no such record exists.

#include <cstdint>
#include <string>
#include <vector>

namespace oblivion {

// Index used for a MAST entry that is not present in the loaded file list.
// rebaseFormID() leaves FormIDs that point at such an entry untouched.
constexpr uint32_t kUnresolvedFormIDIndex = 0xFFFFFFFFu;

// The base game file. It is always loaded first and always takes load order
// index 0, so its own FormIDs are already in load-order space.
inline constexpr const char* kBasePluginFileName = "Oblivion.esm";

// Compares plugin filenames the way the engine does: ASCII case-insensitively.
bool pluginNameEquals(const std::string& lhs, const std::string& rhs);

// Builds the index map for one plugin.
//
// `masters` is the plugin's MAST list in file order, `loadedFileNames` the
// already loaded files in load order (vector position == load-order index) and
// `selfIndex` the load-order index the plugin itself is about to take.
//
// Entry i of the result is the load-order index of master i, and the last entry
// is `selfIndex`. Unresolvable masters map to kUnresolvedFormIDIndex.
std::vector<uint32_t> buildFormIDIndexMap(const std::vector<std::string>& masters,
                                          const std::vector<std::string>& loadedFileNames,
                                          uint32_t selfIndex);

// Translates a single FormID from plugin-relative to load-order space.
// A FormID whose index is missing from the map, points at an unresolved master
// or is zero is returned unchanged.
uint32_t rebaseFormID(uint32_t formID, const std::vector<uint32_t>& indexMap);

}  // namespace oblivion
