#ifndef TESTS_GMST_STRING_TESTS_H
#define TESTS_GMST_STRING_TESTS_H

// ============================================================================
// gmst_string_tests.h — GMST loader-parity host tests
//
// Verifies the string GMST layer and the EXE lookup-only key semantics that
// the asset_manager ESM publish path relies on (game/gmst_defaults.h):
//   - sSpecialization default is present verbatim (EXE 0xA5E1A8)
//   - the four fRegionGenTex* keys are absent from BOTH tables (v60 canon:
//     they exist in Oblivion.esm but the EXE never registers them)
//   - override publish / clear / unknown-key no-op semantics on the string
//     layer mirror the float layer
// Canon: tmp/re/v60_esm_only_keys.md [確定]; RE_EXE_REPORT.md §50.8.
// ============================================================================

#include <string>
#include <vector>

namespace oblivion {
namespace gmst_string {

struct GmstStringTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class GmstStringTests {
public:
    GmstStringTests();

    bool runAllTests();

    const std::vector<GmstStringTestResult>& getResults() const { return results; }
    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void test_string_default_present();
    void test_string_default_verbatim();
    void test_string_unknown_key();
    void test_region_gen_tex_keys_absent();
    void test_string_override_publish();
    void test_string_override_clear();
    void test_string_override_unknown_noop();
    void test_string_override_replacement();
    void test_float_layer_defaults();
    void test_float_layer_override_roundtrip();
    void test_float_layer_unknown_sentinel();

    void record(const std::string& name, bool passed,
                const std::string& msg = "", float ms = 0.0f);

    std::vector<GmstStringTestResult> results;
};

}  // namespace gmst_string
}  // namespace oblivion

#endif  // TESTS_GMST_STRING_TESTS_H
