#pragma once

// Game-path expansion for Oblivion's Data folder layout.
//
// ESM records name models and textures relative to their type folder
// ("Dungeons\Misc\TrigZone01.NIF"), while both the BSA archives and the loose
// Data folder store them under that folder
// ("Meshes\Dungeons\Misc\TrigZone01.NIF"). Every asset lookup therefore has to
// try the type-folder spelling as well, otherwise interior cells resolve zero
// meshes and the world silently renders as clear colour.

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// Type folders that sit directly under Data\ and therefore appear as the first
// path segment of every BSA entry.
inline const std::vector<std::string>& assetRootFolders() {
    static const std::vector<std::string> kRoots = {
        "meshes", "textures", "sound", "menus", "music", "fonts", "shaders", "video", "trees"
    };
    return kRoots;
}

// Expand a game path into the spellings that can resolve, most likely first.
// `normalized` must already have backslashes converted to '/'.
inline std::vector<std::string> buildAssetPathCandidates(const std::string& normalized) {
    std::vector<std::string> candidates;
    candidates.push_back(normalized);

    const std::vector<std::string>& roots = assetRootFolders();

    // Folder roots and extensions are matched case-insensitively: ESM records
    // are inconsistent about it ("TrigZone01.NIF" vs "trigzone01.nif").
    std::string firstSegment = normalized.substr(0, normalized.find('/'));
    std::transform(firstSegment.begin(), firstSegment.end(), firstSegment.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (std::find(roots.begin(), roots.end(), firstSegment) != roots.end()) {
        return candidates;  // Already carries its type folder.
    }

    // Extension-specific root first, then the remaining roots as a safety net
    // for assets whose extension does not reveal where they live.
    std::vector<std::string> ordered;
    const size_t dot = normalized.rfind('.');
    std::string ext = (dot == std::string::npos) ? std::string() : normalized.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".nif" || ext == ".kf" || ext == ".kfa") {
        ordered.emplace_back("meshes");
    } else if (ext == ".dds" || ext == ".tga" || ext == ".bmp") {
        ordered.emplace_back("textures");
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        ordered.emplace_back("sound");
    } else if (ext == ".xml" || ext == ".txt" || ext == ".html" || ext == ".fnt") {
        ordered.emplace_back("menus");
    } else if (ext == ".spt") {
        ordered.emplace_back("trees");
    }
    for (const auto& root : roots) {
        if (std::find(ordered.begin(), ordered.end(), root) == ordered.end()) {
            ordered.emplace_back(root);
        }
    }

    for (const auto& root : ordered) {
        candidates.push_back(root + "/" + normalized);
    }
    return candidates;
}
