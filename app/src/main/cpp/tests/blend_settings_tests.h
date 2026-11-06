// ============================================================================
// blend_settings_tests.h — Host tests for the vanilla BlendSettings table
//
// Verifies the 62-entry ragdoll default table ported from
// tmp/re/v64_section7_remaining.md §2 [確定]:
//   * entry count and section/key composition (HIT/QUADHIT/DEFAULT),
//   * bit-exact float payloads for every float entry,
//   * normalized string payloads, including the corrected sRHand:DEFAULT pair,
//   * case-insensitive lookup (vanilla INI-style registry semantics),
//   * strict "lo, hi" parser acceptance/rejection.
// ============================================================================

#ifndef TESTS_BLEND_SETTINGS_TESTS_H
#define TESTS_BLEND_SETTINGS_TESTS_H

#include <cstdint>
#include <string>
#include <vector>

namespace oblivion {
namespace blend_settings {

struct BlendSettingsTestResult {
    std::string testName;
    bool passed;
    std::string message;
    float durationMs;
};

class BlendSettingsTests {
public:
    BlendSettingsTests() = default;

    bool runAllTests();

    const std::vector<BlendSettingsTestResult>& getResults() const {
        return results;
    }

    int getPassCount() const;
    int getFailCount() const;
    std::string getSummary() const;

private:
    void test_entry_count();
    void test_section_composition();
    void test_type_composition();
    void test_float_bits_all();
    void test_string_values_spot_check();
    void test_default_right_hand_pair();
    void test_bone_name_entries();
    void test_case_insensitive_find();
    void test_getters_type_mismatch();
    void test_parse_pair_accept();
    void test_parse_pair_reject();

    void record(const std::string& name, bool passed,
                const std::string& msg = "", float ms = 0.0f);

    std::vector<BlendSettingsTestResult> results;
};

}  // namespace blend_settings
}  // namespace oblivion

#endif  // TESTS_BLEND_SETTINGS_TESTS_H
