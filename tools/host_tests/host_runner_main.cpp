#include <cstdio>
#include <cstdlib>
#include <string>
#include "tests/script_vm_tests.h"
#include "tests/phase30_integration_test.h"
#include "tests/phase45_unit_tests.h"
#include "tests/phase48_stress_test.h"
#include "tests/phase48_integration_test.h"
#include "tests/watr_decode_tests.h"
#include "tests/xtel_decode_tests.h"
#include "tests/weather_transition_tests.h"
#include "tests/interior_cell_tests.h"
#include "tests/gamestate_report_tests.h"
#include "tests/asset_path_tests.h"
#include "tests/mesh_builder_tests.h"
#include "tests/menu_xml_tests.h"
#include "tests/menu_fade_tests.h"
#include "tests/fatigue_system_tests.h"
#include "tests/combat_formulas_tests.h"
#include "tests/phase69_persistence_tests.h"
#include "tests/plugin_list_tests.h"
#include "tests/formid_index_tests.h"
#include "tests/lod_block_tests.h"
#include "tests/enchanting_system_tests.h"

// Print a suite summary and report whether it passed.
static void runSuite(const char* name, bool ok, const std::string& summary, int& failed) {
    std::printf("\n--- SUITE: %s ---\n", name);
    std::printf("%s\n", summary.c_str());
    if (!ok) {
        failed++;
        std::printf("--- SUITE RESULT: %s FAILED ---\n", name);
    } else {
        std::printf("--- SUITE RESULT: %s PASSED ---\n", name);
    }
}

int main() {
    int failed = 0;
    {
        ScriptVMTests t;
        const bool ok = t.runAllTests();
        runSuite("ScriptVMTests", ok, t.getSummary(), failed);
    }
    {
        Phase45UnitTests t;
        const bool ok = t.runAllTests();
        runSuite("Phase45UnitTests", ok, t.getSummary(), failed);
    }
    {
        Phase48StressTest t;
        const bool ok = t.runAllTests();
        runSuite("Phase48StressTest", ok, t.getSummary(), failed);
    }
    {
        Phase48IntegrationTest t;
        const bool ok = t.runAllTests();
        runSuite("Phase48IntegrationTest", ok, t.getSummary(), failed);
    }
    {
        WatrDecodeTests t;
        const bool ok = t.runAllTests();
        runSuite("WatrDecodeTests", ok, t.getSummary(), failed);
    }
    {
        XtelDecodeTests t;
        const bool ok = t.runAllTests();
        runSuite("XtelDecodeTests", ok, t.getSummary(), failed);
    }
    {
        WeatherTransitionTests t;
        const bool ok = t.runAllTests();
        runSuite("WeatherTransitionTests", ok, t.getSummary(), failed);
    }
    {
        InteriorCellTests t;
        const bool ok = t.runAllTests();
        runSuite("InteriorCellTests", ok, t.getSummary(), failed);
    }
    {
        GameStateReportTests t;
        const bool ok = t.runAllTests();
        runSuite("GameStateReportTests", ok, t.getSummary(), failed);
    }
    {
        AssetPathTests t;
        const bool ok = t.runAllTests();
        runSuite("AssetPathTests", ok, t.getSummary(), failed);
    }
    {
        MeshBuilderTests t;
        const bool ok = t.runAllTests();
        runSuite("MeshBuilderTests", ok, t.getSummary(), failed);
    }
    {
        MenuXmlTests t;
        const bool ok = t.runAllTests();
        runSuite("MenuXmlTests", ok, t.getSummary(), failed);
    }
    {
        MenuFadeTests t;
        const bool ok = t.runAllTests();
        runSuite("MenuFadeTests", ok, t.getSummary(), failed);
    }
    {
        FatigueSystemTests t;
        const bool ok = t.runAllTests();
        runSuite("FatigueSystemTests", ok, t.getSummary(), failed);
    }
    {
        CombatFormulasTests t;
        const bool ok = t.runAllTests();
        runSuite("CombatFormulasTests", ok, t.getSummary(), failed);
    }
    {
        Phase69PersistenceTests t;
        const bool ok = t.runAllTests();
        runSuite("Phase69PersistenceTests", ok, t.getSummary(), failed);
    }
    {
        PluginListTests t;
        const bool ok = t.runAllTests();
        runSuite("PluginListTests", ok, t.getSummary(), failed);
    }
    {
        FormIDIndexTests t;
        const bool ok = t.runAllTests();
        runSuite("FormIDIndexTests", ok, t.getSummary(), failed);
    }
    {
        EnchantingSystemTests t;
        const bool ok = t.runAllTests();
        runSuite("EnchantingSystemTests", ok, t.getSummary(), failed);
    }
    {
        // Real Oblivion assets are not redistributable; the suite skips itself when
        // OBLIVION_ASSET_BASE does not point at an asset tree containing the test NIFs.
        const char* assetBase = std::getenv("OBLIVION_ASSET_BASE");
        Phase30IntegrationTest t;
        const bool ok = t.runAllTests(assetBase != nullptr ? assetBase : "");
        runSuite("Phase30IntegrationTest", ok, t.getSummary(), failed);
    }
    {
        // Needs the extracted meshes\landscape\lod blocks; without them (see
        // OBLIVION_LOD_NIF_DIR) the suite records a SKIP entry and passes.
        const char* assetBase = std::getenv("OBLIVION_ASSET_BASE");
        LodBlockTests t;
        const bool ok = t.runAllTests(assetBase != nullptr ? assetBase : "");
        runSuite("LodBlockTests", ok, t.getSummary(), failed);
    }
    std::printf("\n=== HOST RUNNER: %s (%d suite(s) failed) ===\n", failed ? "FAILED" : "ALL GREEN", failed);
    return failed != 0 ? 1 : 0;
}
