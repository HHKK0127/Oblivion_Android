#include <cstdio>
#include <cstdlib>
#include <string>
#include "tests/script_vm_tests.h"
#include "tests/phase30_integration_test.h"
#include "tests/phase45_unit_tests.h"
#include "tests/phase48_stress_test.h"
#include "tests/phase48_integration_test.h"
#include "tests/watr_decode_tests.h"
#include "tests/weather_transition_tests.h"

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
        WeatherTransitionTests t;
        const bool ok = t.runAllTests();
        runSuite("WeatherTransitionTests", ok, t.getSummary(), failed);
    }
    {
        // Real Oblivion assets are not redistributable; the suite skips itself when
        // OBLIVION_ASSET_BASE does not point at an asset tree containing the test NIFs.
        const char* assetBase = std::getenv("OBLIVION_ASSET_BASE");
        Phase30IntegrationTest t;
        const bool ok = t.runAllTests(assetBase != nullptr ? assetBase : "");
        runSuite("Phase30IntegrationTest", ok, t.getSummary(), failed);
    }
    std::printf("\n=== HOST RUNNER: %s (%d suite(s) failed) ===\n", failed ? "FAILED" : "ALL GREEN", failed);
    return failed != 0 ? 1 : 0;
}
