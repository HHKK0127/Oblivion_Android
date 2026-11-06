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
#include "tests/msvc_lcg_tests.h"
#include "tests/phase69_persistence_tests.h"
#include "tests/plugin_list_tests.h"
#include "tests/formid_index_tests.h"
#include "tests/lod_block_tests.h"
#include "tests/object_lod_tests.h"
#include "tests/enchanting_system_tests.h"
#include "tests/oblivion_ini_tests.h"
#include "tests/settings_manager_tests.h"
#include "tests/merchant_tests.h"
#include "tests/merchant_persistence_tests.h"
#include "tests/condition_tests.h"
#include "tests/ai_package_tests.h"
#include "tests/barter_price_tests.h"
#include "tests/blend_settings_tests.h"
#include "tests/gmst_string_tests.h"
#include "tests/script_fn_tests.h"
#include "tests/topic_resolve_tests.h"
#include "tests/player_character_state_tests.h"
#include "tests/dds_upload_tests.h"
#include "tests/fader_slot_array_tests.h"
#include "tests/input_router_tests.h"

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
        FaderSlotArrayTests t;
        const bool ok = t.runAllTests();
        runSuite("FaderSlotArrayTests", ok, t.getSummary(), failed);
    }
    {
        InputRouterTests t;
        const bool ok = t.runAllTests();
        runSuite("InputRouterTests", ok, t.getSummary(), failed);
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
        MsvcLcgTests t;
        const bool ok = t.runAllTests();
        runSuite("MsvcLcgTests", ok, t.getSummary(), failed);
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
        // Pure string parsing against the compiled-in Oblivion.ini defaults; no
        // game assets or device state are needed.
        OblivionIniTests t;
        const bool ok = t.runAllTests();
        runSuite("OblivionIniTests", ok, t.getSummary(), failed);
    }
    {
        // Layering of settings.txt (owned keys) over Oblivion.ini over shipped
        // defaults. Writes only into the gitignored tmp/ scratch tree, and removes
        // it again when every check passes.
        SettingsManagerTests t;
        const bool ok = t.runAllTests();
        runSuite("SettingsManagerTests", ok, t.getSummary(), failed);
    }
    {
        // Merchant layer: item catalog, ESM CNTO stock, barter arithmetic and the
        // player inventory helpers. Everything runs against a small synthetic ESM
        // built in memory, so no Oblivion assets are needed.
        MerchantTests t;
        const bool ok = t.runAllTests();
        runSuite("MerchantTests", ok, t.getSummary(), failed);
    }
    {
        // MERC save section: a shopkeeper's purse and stock survive a save/load
        // round trip instead of resetting to the plugin defaults. Uses two
        // scratch slots (92/93) and no Oblivion assets.
        MerchantPersistenceTests t;
        const bool ok = t.runAllTests();
        runSuite("MerchantPersistenceTests", ok, t.getSummary(), failed);
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
    {
        // Synthetic .lod parser invariants (v255 layout, EOF-walk rules); no
        // extracted assets needed.
        ObjectLodTests t;
        const bool ok = t.runAllTests();
        runSuite("ObjectLodTests", ok, t.getSummary(), failed);
    }
    {
        // CTDA condition evaluator (v258 port): comparison matrix, OR-group
        // state machine, operand routing, table metadata.
        ConditionTests t;
        const bool ok = t.runAllTests();
        runSuite("ConditionTests", ok, t.getSummary(), failed);
    }
    {
        // AI package first-match selection chain (RE canon 0x569020 port):
        // schedule windows, day mask, CTDA acceptance rules.
        AiPackageTests t;
        const bool ok = t.runAllTests();
        runSuite("AiPackageTests", ok, t.getSummary(), failed);
    }

        {
        // Canonical RE-based barter pricing known-values (barter_price_tests.cpp).
        BarterPriceTests t;
        const bool ok = t.runAllTests();
        runSuite("BarterPriceTests", ok, t.getSummary(), failed);
        }

        {
        // Vanilla BSBlendSettings default table, 62 entries (blend_settings_tests.cpp).
        oblivion::blend_settings::BlendSettingsTests t;
        const bool ok = t.runAllTests();
        runSuite("BlendSettingsTests", ok, t.getSummary(), failed);
        }

        {
        // GMST loader parity: string layer + EXE-unregistered key checks (gmst_string_tests.cpp).
        oblivion::gmst_string::GmstStringTests t;
        const bool ok = t.runAllTests();
        runSuite("GmstStringTests", ok, t.getSummary(), failed);
        }

        {
        // Phase 68 script handlers: Evp/PlaySound/CrimeGold/Kill/Resurrect (script_fn_tests.cpp).
        oblivion::script_fn::ScriptFnTests t;
        const bool ok = t.runAllTests();
        runSuite("ScriptFnTests", ok, t.getSummary(), failed);
        }

        {
        // Topic_Resolve parity: scan/lottery/priority-stop/recursion branch (topic_resolve_tests.cpp).
        oblivion::dialogue::TopicResolveTests t;
        const bool ok = t.runAllTests();
        runSuite("TopicResolveTests", ok, t.getSummary(), failed);
        }

        {
        // V72 global-state DAT_00B333C4 parity: layout/ctor-init/band accessors (player_character_state_tests.cpp).
        game::PlayerCharacterStateTests t;
        const bool ok = t.runAllTests();
        runSuite("PlayerCharacterStateTests", ok, t.getSummary(), failed);
        }
        {
            // Synthetic DXT1 fixture: parser payload parity and the host fallback
            // when no S3TC extension is advertised by the GL stub.
            DdsUploadTests t;
            const bool ok = t.runAllTests();
            runSuite("DdsUploadTests", ok, t.getSummary(), failed);
        }

        std::printf("\n=== HOST RUNNER: %s (%d suite(s) failed) ===\n", failed ? "FAILED" : "ALL GREEN", failed);
    return failed != 0 ? 1 : 0;
}
