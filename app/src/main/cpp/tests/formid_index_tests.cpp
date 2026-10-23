// Tests for assets/formid_index.{h,cpp}. These cover the load-order FormID
// scheme: what a FormID index means inside a plugin file versus in the engine's
// file list. See formid_index_tests.h.

#include "formid_index_tests.h"

#include "../assets/formid_index.h"

#include <cstdio>
#include <string>
#include <vector>

using oblivion::buildFormIDIndexMap;
using oblivion::kUnresolvedFormIDIndex;
using oblivion::pluginNameEquals;
using oblivion::rebaseFormID;

namespace {

std::string hex(uint32_t value) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08X", value);
    return buffer;
}

std::string describeIndexMap(const std::vector<uint32_t>& indexMap) {
    std::string out = "[";
    for (size_t i = 0; i < indexMap.size(); i++) {
        if (i) out += ", ";
        out += indexMap[i] == kUnresolvedFormIDIndex ? "unresolved"
                                                     : std::to_string(indexMap[i]);
    }
    out += "]";
    return out;
}

}  // namespace

void FormIDIndexTests::record(const std::string& name, bool passed,
                              const std::string& msg) {
    results.push_back({name, passed, msg});
}

int FormIDIndexTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) if (r.passed) count++;
    return count;
}

int FormIDIndexTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string FormIDIndexTests::getSummary() const {
    std::string summary = "FormIDIndexTest Results\n";
    summary += "Total: " + std::to_string(results.size()) +
               " | Pass: " + std::to_string(getPassCount()) +
               " | Fail: " + std::to_string(getFailCount()) + "\n";
    for (const auto& r : results) {
        summary += std::string(r.passed ? "[PASS] " : "[FAIL] ") + r.testName;
        if (!r.message.empty()) summary += " - " + r.message;
        summary += "\n";
    }
    return summary;
}

bool FormIDIndexTests::runAllTests() {
    results.clear();

    // -------------------------------------------------------------- name match
    {
        record("plugin names compare case-insensitively",
               pluginNameEquals("Oblivion.esm", "OBLIVION.ESM") &&
                   pluginNameEquals("Knights.esp", "knights.ESP"),
               "ok");
    }

    {
        record("plugin names of different length or spelling differ",
               !pluginNameEquals("Mod.esp", "Mod.esp.bak") &&
                   !pluginNameEquals("Mod.esp", "Mod2.esp") &&
                   !pluginNameEquals("", "Mod.esp") &&
                   pluginNameEquals("", ""),
               "ok");
    }

    // ------------------------------------------------------------ index mapping
    {
        // Oblivion.esm has no masters and is file 0, so its map is the identity.
        const auto indexMap = buildFormIDIndexMap({}, {"Oblivion.esm"}, 0);
        record("the base file maps onto itself",
               indexMap == std::vector<uint32_t>{0} && describeIndexMap(indexMap) == "[0]",
               describeIndexMap(indexMap));
    }

    {
        // Masters in order, then the plugin's own index last.
        const auto indexMap =
            buildFormIDIndexMap({"Oblivion.esm", "Knights.esp"},
                                {"Oblivion.esm", "Mod.esp", "Knights.esp"}, 3);
        record("masters resolve to their load-order position",
               indexMap == std::vector<uint32_t>{0, 2, 3}, describeIndexMap(indexMap));
    }

    {
        const auto indexMap = buildFormIDIndexMap({"Missing.esp"}, {"Oblivion.esm"}, 1);
        record("an absent master is marked unresolved",
               indexMap.size() == 2 && indexMap[0] == kUnresolvedFormIDIndex &&
                   indexMap[1] == 1,
               describeIndexMap(indexMap));
    }

    {
        const auto indexMap =
            buildFormIDIndexMap({"KNIGHTS.ESP"}, {"Oblivion.esm", "Knights.esp"}, 1);
        record("masters match loaded files case-insensitively",
               indexMap.size() == 2 && indexMap[0] == 1, describeIndexMap(indexMap));
    }

    // ------------------------------------------------------------- rebasing
    {
        // A plugin that lists Oblivion.esm as its only master, loaded second.
        // Its own records carry index 1 (= masters.size()) and stay put, while
        // references to Oblivion.esm carry index 0 and must stay at 0.
        const auto indexMap = buildFormIDIndexMap({"Oblivion.esm"}, {"Oblivion.esm"}, 1);
        const uint32_t ownRecord = rebaseFormID(0x0100ABCD, indexMap);
        const uint32_t masterRef = rebaseFormID(0x0000ABCD, indexMap);
        record("a plugin keeps its own index and its master references",
               ownRecord == 0x0100ABCD && masterRef == 0x0000ABCD,
               "own=" + hex(ownRecord) + " master=" + hex(masterRef));
    }

    {
        // The same plugin loaded third: its own index moves, the master's does not.
        const auto indexMap = buildFormIDIndexMap({"Oblivion.esm"},
                                                 {"Oblivion.esm", "A.esp"}, 2);
        const uint32_t ownRecord = rebaseFormID(0x0100ABCD, indexMap);
        const uint32_t masterRef = rebaseFormID(0x0000ABCD, indexMap);
        record("a plugin's own index follows its load position",
               ownRecord == 0x0200ABCD && masterRef == 0x0000ABCD,
               "own=" + hex(ownRecord) + " master=" + hex(masterRef));
    }

    {
        // A second master sits between Oblivion.esm and the plugin in load order,
        // so index 1 in the file means file 2 in the engine.
        const auto indexMap =
            buildFormIDIndexMap({"Oblivion.esm", "Knights.esp"},
                                {"Oblivion.esm", "Mod.esp", "Knights.esp"}, 3);
        const uint32_t masterRef = rebaseFormID(0x0100BEEF, indexMap);
        const uint32_t ownRecord = rebaseFormID(0x0200BEEF, indexMap);
        record("a reordered master is followed to its load-order index",
               masterRef == 0x0200BEEF && ownRecord == 0x0300BEEF,
               "master=" + hex(masterRef) + " own=" + hex(ownRecord));
    }

    {
        const auto indexMap =
            buildFormIDIndexMap({"Oblivion.esm", "Missing.esp"}, {"Oblivion.esm"}, 1);
        const uint32_t masterRef = rebaseFormID(0x0000ABCD, indexMap);
        const uint32_t missingRef = rebaseFormID(0x0100ABCD, indexMap);
        const uint32_t ownRecord = rebaseFormID(0x0200ABCD, indexMap);
        record("an unresolved master leaves its FormIDs alone",
               masterRef == 0x0000ABCD && missingRef == 0x0100ABCD &&
                   ownRecord == 0x0100ABCD,
               "master=" + hex(masterRef) + " missing=" + hex(missingRef) +
                   " own=" + hex(ownRecord));
    }

    {
        const auto indexMap = buildFormIDIndexMap({}, {"Oblivion.esm"}, 0);
        record("the base file's FormIDs are not touched",
               rebaseFormID(0x00001234, indexMap) == 0x00001234 &&
                   rebaseFormID(0x0000FFFF, indexMap) == 0x0000FFFF,
               hex(rebaseFormID(0x00001234, indexMap)));
    }

    {
        const auto indexMap = buildFormIDIndexMap({}, {"Oblivion.esm"}, 0);
        // Index 1 is not part of Oblivion.esm's own space, so the data is left as
        // it is rather than being folded into an index the file cannot mean.
        record("an index outside the plugin's space is left as it is",
               rebaseFormID(0x01000001, indexMap) == 0x01000001 &&
                   rebaseFormID(0xFF00BEEF, indexMap) == 0xFF00BEEF,
               hex(rebaseFormID(0x01000001, indexMap)));
    }

    {
        const auto indexMap = buildFormIDIndexMap({"Oblivion.esm"}, {"Oblivion.esm"}, 1);
        record("a null FormID stays null",
               rebaseFormID(0x00000000, indexMap) == 0x00000000, "0x00000000");
    }

    {
        const auto indexMap = buildFormIDIndexMap({"Oblivion.esm"}, {"Oblivion.esm"}, 3);
        // Only the index changes: the 24-bit record number is preserved exactly.
        record("the low 24 bits survive the rebase",
               rebaseFormID(0x01FFFFFF, indexMap) == 0x03FFFFFF &&
                   rebaseFormID(0x01000001, indexMap) == 0x03000001,
               hex(rebaseFormID(0x01FFFFFF, indexMap)));
    }

    {
        // Re-running the translation against the base file's identity map must not
        // move anything: startup loads Oblivion.esm through the same code path.
        const auto indexMap = buildFormIDIndexMap({}, {"Oblivion.esm"}, 0);
        uint32_t id = 0x0000BEEF;
        id = rebaseFormID(id, indexMap);
        id = rebaseFormID(id, indexMap);
        record("repeating the base file's translation is harmless",
               id == 0x0000BEEF, hex(id));
    }

    return getFailCount() == 0;
}
