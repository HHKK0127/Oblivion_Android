#pragma once

// Game-path expansion for Oblivion's Data folder layout.
//
// ESM records name models and textures relative to their type folder
// ("Dungeons\Misc\TrigZone01.NIF"), while both the BSA archives and the loose
// Data folder store them under that folder
// ("Meshes\Dungeons\Misc\TrigZone01.NIF"). Every asset lookup therefore has to
// try the type-folder spelling as well, otherwise interior cells resolve zero
// meshes and the world silently renders as clear colour.
//
// The expansion itself lives in Rust (oblivion-re-tools, crates/assets-core)
// behind the assets_ffi C ABI. These declarations are unchanged from the original
// header-only implementation so existing callers keep working.

#include <string>
#include <vector>

// Type folders that sit directly under Data\ and therefore appear as the first
// path segment of every BSA entry, in lookup-priority order.
const std::vector<std::string>& assetRootFolders();

// Expand a game path into the spellings that can resolve, most likely first.
// `normalized` must already have backslashes converted to '/'. An already-rooted
// path yields exactly one candidate.
//
// Returns the original spelling alone when the ABI refuses the call: a lookup that
// only tries what the caller asked for can still hit a loose file, while a wrong
// candidate list would send the resolver after paths that cannot exist.
std::vector<std::string> buildAssetPathCandidates(const std::string& normalized);

