#include "formid_index.h"

#include <cctype>

namespace oblivion {

bool pluginNameEquals(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (size_t i = 0; i < lhs.size(); i++) {
        const unsigned char a = static_cast<unsigned char>(lhs[i]);
        const unsigned char b = static_cast<unsigned char>(rhs[i]);
        if (a == b) continue;
        if (std::tolower(a) != std::tolower(b)) return false;
    }
    return true;
}

std::vector<uint32_t> buildFormIDIndexMap(const std::vector<std::string>& masters,
                                          const std::vector<std::string>& loadedFileNames,
                                          uint32_t selfIndex) {
    std::vector<uint32_t> indexMap;
    indexMap.reserve(masters.size() + 1);
    for (const std::string& master : masters) {
        uint32_t found = kUnresolvedFormIDIndex;
        for (size_t i = 0; i < loadedFileNames.size(); i++) {
            if (pluginNameEquals(master, loadedFileNames[i])) {
                found = static_cast<uint32_t>(i);
                break;
            }
        }
        indexMap.push_back(found);
    }
    // A plugin's own records are addressed by the index just past its masters.
    indexMap.push_back(selfIndex);
    return indexMap;
}

uint32_t rebaseFormID(uint32_t formID, const std::vector<uint32_t>& indexMap) {
    if (formID == 0) return 0;
    const uint32_t pluginIndex = (formID >> 24) & 0xFFu;
    if (pluginIndex >= indexMap.size()) return formID;
    const uint32_t loadOrderIndex = indexMap[pluginIndex];
    if (loadOrderIndex == kUnresolvedFormIDIndex) return formID;
    return ((loadOrderIndex & 0xFFu) << 24) | (formID & 0x00FFFFFFu);
}

}  // namespace oblivion
