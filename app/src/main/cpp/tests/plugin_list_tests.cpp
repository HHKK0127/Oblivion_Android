// Tests for the plugins.txt grammar used by the launcher's Data Files screen and
// by the engine's startup load order. See plugin_list_tests.h.

#include "plugin_list_tests.h"

#include "../assets/plugin_list.h"

#include <string>
#include <vector>

using oblivion::PluginListEntry;

namespace {

std::vector<std::string> namesOf(const std::vector<PluginListEntry>& entries) {
    std::vector<std::string> names;
    for (const auto& entry : entries) names.push_back(entry.name);
    return names;
}

bool sameNames(const std::vector<PluginListEntry>& entries,
               const std::vector<std::string>& expected) {
    return namesOf(entries) == expected;
}

std::string describe(const std::vector<std::string>& names) {
    std::string out;
    for (size_t i = 0; i < names.size(); i++) {
        if (i) out += ", ";
        out += names[i];
    }
    return out;
}

std::vector<PluginListEntry> makeEntries(const std::vector<std::string>& names,
                                        const std::vector<bool>& active) {
    std::vector<PluginListEntry> entries;
    for (size_t i = 0; i < names.size(); i++) {
        entries.push_back({names[i], i < active.size() ? active[i] : true});
    }
    return entries;
}

}  // namespace

void PluginListTests::record(const std::string& name, bool passed,
                             const std::string& msg) {
    results.push_back({name, passed, msg});
}

int PluginListTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) if (r.passed) count++;
    return count;
}

int PluginListTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string PluginListTests::getSummary() const {
    std::string summary = "PluginListTest Results\n";
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

bool PluginListTests::runAllTests() {
    results.clear();

    // ---------------------------------------------------------------- parsing
    {
        const auto entries = oblivion::parsePluginList(
            "Oblivion.esm\nUnofficial Oblivion Patch.esp\n");
        record("plain list keeps every plugin active",
               entries.size() == 2 && entries[0].active && entries[1].active &&
                   sameNames(entries, {"Oblivion.esm", "Unofficial Oblivion Patch.esp"}),
               describe(namesOf(entries)));
    }

    {
        // A launcher-written file marks active plugins with '*', so an unmarked
        // line is a plugin the user switched off.
        const auto entries = oblivion::parsePluginList(
            "*Oblivion.esm\nSomeMod.esp\n*OtherMod.esp\n");
        record("marked list disables unmarked lines",
               entries.size() == 3 && entries[0].active && !entries[1].active &&
                   entries[2].active,
               entries.size() == 3 ? (entries[1].active ? "unmarked line stayed active"
                                                        : "ok")
                                  : "wrong entry count");
    }

    {
        const auto entries = oblivion::parsePluginList(
            "# written by a mod manager\n\n   \n\t\n*Oblivion.esm\n");
        record("comments and blank lines are skipped",
               entries.size() == 1 && entries[0].name == "Oblivion.esm",
               describe(namesOf(entries)));
    }

    {
        const auto entries = oblivion::parsePluginList(
            "*Oblivion.esm\r\n*Mod.esp\r\n");
        record("carriage returns are trimmed",
               entries.size() == 2 && entries[0].name == "Oblivion.esm" &&
                   entries[1].name == "Mod.esp",
               describe(namesOf(entries)));
    }

    {
        const auto entries = oblivion::parsePluginList(
            "\xEF\xBB\xBFOblivion.esm\n");
        record("utf-8 bom is trimmed",
               entries.size() == 1 && entries[0].name == "Oblivion.esm",
               describe(namesOf(entries)));
    }

    {
        const auto entries = oblivion::parsePluginList("  *  Mod.esp  \n");
        record("marker and surrounding blanks are trimmed",
               entries.size() == 1 && entries[0].name == "Mod.esp" &&
                   entries[0].active,
               describe(namesOf(entries)));
    }

    {
        const auto entries = oblivion::parsePluginList("Mod.esp\nMOD.ESP\nMod.Esp\n");
        record("duplicate names are dropped case-insensitively",
               entries.size() == 1 && entries[0].name == "Mod.esp",
               describe(namesOf(entries)));
    }

    {
        const auto entries = oblivion::parsePluginList("*\n   \n");
        record("a bare marker is not a plugin", entries.empty(),
               std::to_string(entries.size()) + " entries");
    }

    {
        const auto entries = oblivion::parsePluginList("");
        record("empty input yields nothing", entries.empty(),
               std::to_string(entries.size()) + " entries");
    }

    {
        const auto entries = oblivion::parsePluginList("# only a comment\n");
        record("comment-only file yields nothing", entries.empty(),
               std::to_string(entries.size()) + " entries");
    }

    {
        const auto entries = oblivion::parsePluginList("*Oblivion.esm\n*B.esp\nA.esp\n");
        const auto active = oblivion::activePluginNames(entries);
        record("active names keep their file order",
               active.size() == 2 && active[0] == "Oblivion.esm" && active[1] == "B.esp",
               describe(active));
    }

    // ------------------------------------------------------------ serializing
    {
        const auto entries = makeEntries({"Oblivion.esm", "Mod.esp"}, {true, false});
        const std::string text = oblivion::serializePluginList(entries);
        record("active entries are written with a marker",
               text == "*Oblivion.esm\nMod.esp\n",
               text.empty() ? "empty output" : text);
    }

    {
        const std::string text = oblivion::serializePluginList({});
        record("an empty selection serialises to an empty file", text.empty(),
               std::to_string(text.size()) + " bytes");
    }

    {
        const auto original = oblivion::parsePluginList("*Oblivion.esm\nMod.esp\n");
        const auto roundTripped =
            oblivion::parsePluginList(oblivion::serializePluginList(original));
        record("round trip preserves a marked list",
               roundTripped.size() == original.size() &&
                   roundTripped[0].name == "Oblivion.esm" && roundTripped[0].active &&
                   roundTripped[1].name == "Mod.esp" && !roundTripped[1].active,
               describe(namesOf(roundTripped)));
    }

    {
        const auto original = oblivion::parsePluginList("Oblivion.esm\nMod.esp\n");
        const auto roundTripped =
            oblivion::parsePluginList(oblivion::serializePluginList(original));
        record("round trip preserves a plain list",
               roundTripped.size() == 2 && roundTripped[0].active &&
                   roundTripped[1].active,
               describe(namesOf(roundTripped)));
    }

    {
        const auto entries = makeEntries({"A.esp", "B.esp", "C.esp"},
                                        {true, true, true});
        const std::string text = oblivion::serializePluginList(entries);
        record("every line of a full selection is marked",
               text == "*A.esp\n*B.esp\n*C.esp\n",
               text.empty() ? "empty output" : text);
    }

    // ------------------------------------------------------------- load order
    {
        auto entries = makeEntries(
            {"Mod.esp", "Oblivion.esm", "AnotherMod.esp", "DLCShiveringIsles.esp"},
            {true, true, true, true});
        oblivion::sortPluginLoadOrder(entries);
        record("Oblivion.esm sorts first",
               !entries.empty() && entries[0].name == "Oblivion.esm",
               entries.empty() ? "no entries" : entries[0].name);
    }

    {
        auto entries = makeEntries(
            {"Mod.esp", "Oblivion.esm", "DLCBattlehornCastle.esp", "Knights.esp",
             "Unofficial Oblivion Patch.esp"},
            {true, true, true, true, true});
        oblivion::sortPluginLoadOrder(entries);
        record("masters sort before plugins",
               sameNames(entries, {"Oblivion.esm", "DLCBattlehornCastle.esp",
                                   "Knights.esp", "Mod.esp",
                                   "Unofficial Oblivion Patch.esp"}),
               describe(namesOf(entries)));
    }

    {
        auto entries = makeEntries({"zeta.esp", "Alpha.esp", "beta.esp"},
                                   {true, true, true});
        oblivion::sortPluginLoadOrder(entries);
        record("plugins sort alphabetically ignoring case",
               sameNames(entries, {"Alpha.esp", "beta.esp", "zeta.esp"}),
               describe(namesOf(entries)));
    }

    {
        auto entries = makeEntries({"Knights.esp", "Oblivion.esm", "DLCShiveringIsles.esp"},
                                   {true, true, true});
        oblivion::sortPluginLoadOrder(entries);
        record("masters sort alphabetically among themselves",
               sameNames(entries, {"Oblivion.esm", "DLCShiveringIsles.esp",
                                   "Knights.esp"}),
               describe(namesOf(entries)));
    }

    {
        // The sort is stable, so a plugin and a master that only differ in case
        // keep the order they arrived in rather than swapping unpredictably.
        auto entries = makeEntries({"Mod.esp", "MOD.ESP"}, {true, false});
        oblivion::sortPluginLoadOrder(entries);
        record("sort keeps the incoming order for equal names",
               entries.size() == 2 && entries[0].name == "Mod.esp" &&
                   entries[0].active && !entries[1].active,
               describe(namesOf(entries)));
    }

    {
        record("isMasterFileName accepts .esm in any case",
               oblivion::isMasterFileName("Oblivion.ESM") &&
                   oblivion::isMasterFileName("Knights.esm"),
               "ok");
    }

    {
        record("isMasterFileName rejects plugins",
               !oblivion::isMasterFileName("Mod.esp") &&
                   !oblivion::isMasterFileName("Mod.esm.bak") &&
                   !oblivion::isMasterFileName("esm"),
               "ok");
    }

    return getFailCount() == 0;
}
