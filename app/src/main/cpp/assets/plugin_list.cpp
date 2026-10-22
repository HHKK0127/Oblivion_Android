#include "plugin_list.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include <android/log.h>

#define LOG_TAG "PluginList"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace oblivion {

namespace {

const char* kOblivionEsm = "oblivion.esm";

std::string toLower(const std::string& value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (char c : value) {
        lowered.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))));
    }
    return lowered;
}

bool isBlank(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Removes leading and trailing blanks. Also drops a UTF-8 BOM that a Windows
// editor may have left at the start of the file.
std::string trim(const std::string& value) {
    size_t begin = 0;
    if (value.size() >= 3 &&
        static_cast<unsigned char>(value[0]) == 0xEF &&
        static_cast<unsigned char>(value[1]) == 0xBB &&
        static_cast<unsigned char>(value[2]) == 0xBF) {
        begin = 3;
    }
    while (begin < value.size() && isBlank(value[begin])) begin++;
    size_t end = value.size();
    while (end > begin && isBlank(value[end - 1])) end--;
    return value.substr(begin, end - begin);
}

std::vector<std::string> splitLines(const std::string& contents) {
    std::vector<std::string> lines;
    std::string current;
    for (char c : contents) {
        if (c == '\n') {
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

}  // namespace

bool isMasterFileName(const std::string& name) {
    if (name.size() < 4) return false;
    const std::string ext = toLower(name.substr(name.size() - 4));
    return ext == ".esm";
}

std::vector<PluginListEntry> parsePluginList(const std::string& contents) {
    struct RawLine {
        std::string name;
        bool marked;
    };

    std::vector<RawLine> lines;
    std::unordered_set<std::string> seen;
    bool fileUsesMarkers = false;

    for (const std::string& raw : splitLines(contents)) {
        std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;

        bool marked = false;
        if (line[0] == '*') {
            marked = true;
            fileUsesMarkers = true;
            line = trim(line.substr(1));
        }
        if (line.empty()) continue;

        // Filenames are case-insensitive on the platforms Oblivion ships for,
        // so a repeated plugin spelled differently is still a duplicate.
        if (!seen.insert(toLower(line)).second) {
            LOGD("Ignoring duplicate plugin entry: %s", line.c_str());
            continue;
        }

        lines.push_back({line, marked});
    }

    std::vector<PluginListEntry> entries;
    entries.reserve(lines.size());
    for (const auto& line : lines) {
        PluginListEntry entry;
        entry.name = line.name;
        // Without any marker in the file every listed plugin is active, which is
        // how the retail launcher writes it.
        entry.active = fileUsesMarkers ? line.marked : true;
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::string serializePluginList(const std::vector<PluginListEntry>& entries) {
    std::string out;
    for (const auto& entry : entries) {
        if (entry.name.empty()) continue;
        if (entry.active) out.push_back('*');
        out += entry.name;
        out.push_back('\n');
    }
    return out;
}

std::vector<std::string> activePluginNames(const std::vector<PluginListEntry>& entries) {
    std::vector<std::string> names;
    names.reserve(entries.size());
    for (const auto& entry : entries) {
        if (entry.active && !entry.name.empty()) names.push_back(entry.name);
    }
    return names;
}

void sortPluginLoadOrder(std::vector<PluginListEntry>& entries) {
    auto rank = [](const PluginListEntry& entry) {
        const std::string lowered = toLower(entry.name);
        if (lowered == kOblivionEsm) return 0;
        if (isMasterFileName(entry.name)) return 1;
        return 2;
    };

    std::stable_sort(entries.begin(), entries.end(),
                     [&rank](const PluginListEntry& lhs, const PluginListEntry& rhs) {
                         const int lhsRank = rank(lhs);
                         const int rhsRank = rank(rhs);
                         if (lhsRank != rhsRank) return lhsRank < rhsRank;
                         return toLower(lhs.name) < toLower(rhs.name);
                     });
}

}  // namespace oblivion
