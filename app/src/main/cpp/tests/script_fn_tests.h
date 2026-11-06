#ifndef TESTS_SCRIPT_FN_TESTS_H
#define TESTS_SCRIPT_FN_TESTS_H

// Script function tests for the Phase-68 FunctionID band (V75a canon).
// Covers the six handlers registered in script_functions.cpp:
//   Evp            FunctionID 0x9A (0x105E) -> native 0x50AF90 PackageSelect_Main 0x569020
//   PlaySound      FunctionID 0x9B (0x1026) -> native 0x509490 SoundCreate 0x6AE0A0 / SoundPlay 0x6B7190
//   ModCrimeGold   FunctionID 0x9C (0x1076) -> native 0x504D00 (Actor.vt+0x254)
//   SetCrimeGold   FunctionID 0x9D (0x1075) -> native 0x50B4A0
//   KillActor      FunctionID 0x9E (0x108B) -> native 0x501960 Actor::Kill 0x6005F0
//   ResurrectActor FunctionID 0x9F (0x108C) -> native 0x510150 (Actor.vt+0x20C)
// Canon: docs/RE_EXE_REPORT.md 68 / tmp/re/v75a_findings.md

#include <string>
#include <vector>

namespace oblivion {
namespace script_fn {

struct ScriptFnTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class ScriptFnTests {
public:
    ScriptFnTests();

    bool runAllTests();

    const std::vector<ScriptFnTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void test_evp_unresolved_self_ref();
    void test_evp_resets_non_combat();
    void test_evp_preserves_combat();
    void test_playsound_guards();
    void test_crimegold_arg_guards();
    void test_setcrimegold_set_and_clamp();
    void test_modcrimegold_delta();
    void test_killactor_guards();
    void test_killactor_kills_npc();
    void test_resurrectactor_guards();
    void test_kill_resurrect_cycle();

    void record(const std::string& name, bool passed, const std::string& msg = "", float ms = 0.0f);

    std::vector<ScriptFnTestResult> results;
};

} // namespace script_fn
} // namespace oblivion

#endif // TESTS_SCRIPT_FN_TESTS_H
