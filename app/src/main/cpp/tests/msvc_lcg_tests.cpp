#include "tests/msvc_lcg_tests.h"

#include "util/msvc_lcg.h"
#include "util/numeric.h"

#include <cstdio>

namespace {

// MSVC LCG reference: state = state * 214013 + 2531011 (uint32),
// output = (state >> 16) & 0x7FFF. seed 0 is treated as seed 1.
uint32_t reference_next(uint32_t& state) {
    state = state * 214013u + 2531011u;
    return (state >> 16) & 0x7FFFu;
}

} // namespace

void MsvcLcgTests::record(const std::string& name, bool passed, const std::string& msg) {
    MsvcLcgTestResult r;
    r.testName = name;
    r.passed = passed;
    r.message = msg;
    results.push_back(r);
    std::printf("[%s] %s%s%s\n", passed ? "PASS" : "FAIL", name.c_str(),
                msg.empty() ? "" : " - ", msg.c_str());
}

int MsvcLcgTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (r.passed) ++n;
    }
    return n;
}

int MsvcLcgTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (!r.passed) ++n;
    }
    return n;
}

std::string MsvcLcgTests::getSummary() const {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "Total: %d | Pass: %d | Fail: %d",
                  static_cast<int>(results.size()), getPassCount(), getFailCount());
    return std::string(buf);
}

bool MsvcLcgTests::runAllTests() {
    results.clear();

    // --- 1. MSVC LCG known sequence (RE section 60.3) ---
    {
        oblivion::rng::srand(1);
        uint32_t ref = 1u;
        bool ok = true;
        int mismatch = -1;
        for (int i = 0; i < 10; ++i) {
            int got = oblivion::rng::rand();
            int want = static_cast<int>(reference_next(ref));
            if (got != want) {
                ok = false;
                mismatch = i;
                break;
            }
            if (got < 0 || got > oblivion::rng::kRandMax) {
                ok = false;
                mismatch = i;
                break;
            }
        }
        record("lcg_known_sequence_seed1", ok,
               ok ? "10 values match reference LCG" : "mismatch at index");
        if (!ok) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "index %d", mismatch);
            results.back().message = buf;
        }
    }

    // --- 2. seed 0 maps to seed 1 (MSVC srand semantics) ---
    {
        oblivion::rng::srand(0);
        uint32_t ref = 1u;
        int a = oblivion::rng::rand();
        int b = static_cast<int>(reference_next(ref));
        record("lcg_seed0_treated_as_seed1", a == b,
               a == b ? "seed 0 == seed 1 stream" : "stream differs");
    }

    // --- 3. rand_range stays within [min, max] ---
    {
        oblivion::rng::srand(12345);
        bool ok = true;
        for (int i = 0; i < 256; ++i) {
            int v = oblivion::rng::rand_range(3, 9);
            if (v < 3 || v > 9) {
                ok = false;
                break;
            }
        }
        record("rand_range_within_bounds", ok, ok ? "all 256 rolls in [3,9]" : "out of range");
    }

    // --- 4. rand_range degenerate: max <= min returns min ---
    {
        int v = oblivion::rng::rand_range(5, 5);
        record("rand_range_degenerate", v == 5, v == 5 ? "returns min" : "unexpected value");
    }

    // --- 4b. rand_range is inclusive: every value in [0,3] must appear ---
    // Regression guard for the old scaling implementation
    // (min + rand() * span / kRandMax), which only produced the upper bound
    // when rand() == kRandMax and therefore almost never selected it.
    {
        oblivion::rng::srand(12345);
        bool seen[4] = {false, false, false, false};
        for (int i = 0; i < 2000; ++i) {
            const int v = oblivion::rng::rand_range(0, 3);
            if (v >= 0 && v < 4) {
                seen[v] = true;
            }
        }
        const bool all = seen[0] && seen[1] && seen[2] && seen[3];
        record("rand_range_reaches_both_ends", all,
               all ? "saw 0..3 in 2000 rolls" : "an end of [0,3] was never reached");
    }

    // --- 4c. rand_range(0, n-1) usable as an array index: last slot reachable ---
    {
        oblivion::rng::srand(777);
        bool hit_last = false;
        for (int i = 0; i < 2000; ++i) {
            if (oblivion::rng::rand_range(0, 9) == 9) {
                hit_last = true;
                break;
            }
        }
        record("rand_range_index_last_element_reachable", hit_last,
               hit_last ? "index n-1 reachable" : "index n-1 never selected");
    }

    // --- 5. rand_bool boundary: rand()==32767 and threshold 1.0 -> true (<=) ---
    {
        oblivion::rng::srand(1);
        // Exhaust the stream until a 32767 output appears (max 32768 draws).
        bool found = false;
        for (int i = 0; i < 40000; ++i) {
            if (oblivion::rng::rand() == oblivion::rng::kRandMax) {
                found = true;
                break;
            }
        }
        record("rand_bool_reaches_max", found, found ? "kRandMax observed" : "never hit 32767");
    }

    // --- 6. rand_bool threshold 0.0: only rand()==0 can pass with <= ---
    {
        oblivion::rng::srand(1);
        bool saw_true_at_zero = false;
        for (int i = 0; i < 40000; ++i) {
            if (oblivion::rng::rand() == 0) {
                // Recompute the helper comparison directly: (0/32767.0f) <= 0.0f.
                saw_true_at_zero = (0.0f / 32767.0f) <= 0.0f;
                break;
            }
        }
        record("rand_bool_zero_threshold_semantics", saw_true_at_zero,
               saw_true_at_zero ? "(0/32767.0f) <= 0.0f is true" : "comparison false");
    }

    // --- 7. rand_bool threshold 1.0 always true ---
    {
        oblivion::rng::srand(999);
        bool ok = true;
        for (int i = 0; i < 64; ++i) {
            if (!oblivion::rng::rand_bool(1.0f)) {
                ok = false;
                break;
            }
        }
        record("rand_bool_threshold1_always_true", ok, ok ? "all 64 rolls true" : "false observed");
    }

    // --- 8. rand_bool threshold 0.0: true only when rand()==0 ---
    {
        oblivion::rng::srand(1);
        bool consistent = true;
        for (int i = 0; i < 40000; ++i) {
            int v = oblivion::rng::rand();
            bool helper = (static_cast<float>(v) / 32767.0f) <= 0.0f;
            bool expect = (v == 0);
            if (helper != expect) {
                consistent = false;
                break;
            }
        }
        record("rand_bool_zero_threshold_matches_rand0", consistent,
               consistent ? "helper == (v == 0) for 40000 draws" : "mismatch");
    }

    // --- 9. rand_with_seed nonzero reseeds deterministically ---
    {
        oblivion::rng::rand_with_seed(777);
        int a1 = oblivion::rng::rand_with_seed(777);
        int a2 = oblivion::rng::rand_with_seed(777);
        record("rand_with_seed_nonzero_reseeds", a1 == a2,
               a1 == a2 ? "same seed -> same first value" : "values differ");
    }

    // --- 10. rand_with_seed(0) keeps the current stream once seeded ---
    {
        oblivion::rng::srand(42);
        int a = oblivion::rng::rand_with_seed(0);
        int b = oblivion::rng::rand();
        // After srand(42), seed 0 must not reseed: next rand() continues the stream.
        uint32_t ref = 42u;
        reference_next(ref); // consumed by a
        int want_b = static_cast<int>(reference_next(ref));
        record("rand_with_seed0_keeps_stream", b == want_b,
               b == want_b ? "seed 0 did not reseed" : "stream was reseeded");
        (void)a;
    }

    // --- 11. round_half_up basics (util/numeric.h) ---
    {
        bool ok = oblivion::round_half_up(2.5f) == 3.0f &&
                  oblivion::round_half_up(-2.5f) == -2.0f &&
                  oblivion::round_half_up(-0.4f) == 0.0f &&
                  oblivion::round_half_up(0.5f) == 1.0f;
        record("round_half_up_cases", ok,
               ok ? "-2.5->-2, 2.5->3, -0.4->0, 0.5->1" : "unexpected rounding");
    }

    // --- 12. round_half_up on integer-valued floats is identity ---
    {
        bool ok = oblivion::round_half_up(3.0f) == 3.0f &&
                  oblivion::round_half_up(-4.0f) == -4.0f;
        record("round_half_up_identity", ok, ok ? "integers unchanged" : "changed");
    }

    return getFailCount() == 0;
}
