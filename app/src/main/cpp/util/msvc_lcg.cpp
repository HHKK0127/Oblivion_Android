#include "msvc_lcg.h"

#include <ctime>

namespace oblivion {
namespace rng {

namespace {

// Single process-global stream (matches the original executable).
uint32_t g_state = 0;

// True until the first explicit seed or lazy time() seed.
bool g_unseeded = true;

} // namespace

void srand(uint32_t seed) {
    // MSVC treats seed 0 as seed 1.
    g_state = (seed == 0) ? 1u : seed;
    g_unseeded = false;
}

int rand() {
    // MSVC LCG: state = state * 214013 + 2531011 (uint32 wrap).
    g_state = g_state * 214013u + 2531011u;
    return static_cast<int>((g_state >> 16) & 0x7FFFu);
}

int rand_with_seed(uint32_t seed) {
    if (seed != 0) {
        srand(seed);
    } else if (g_unseeded) {
        // Deferred first-time seed: only when nothing has seeded yet.
        srand(static_cast<uint32_t>(std::time(nullptr)));
    }
    return rand();
}

int rand_range(int min, int max) {
    if (max <= min) {
        return min;
    }
    // Inclusive on both ends, like std::uniform_int_distribution. Scaling by
    // rand() * span / kRandMax cannot work: max is only reached when
    // rand() == kRandMax (probability 1/32768), so callers such as
    // rand_range(0, size - 1) would almost never pick the last element.
    // The modulo bias is bounded by span / 32768 and is negligible here.
    const int span = max - min + 1;
    return min + rand() % span;
}

bool rand_bool(float threshold) {
    // FUN_0047FB70 compares with <= (confirmed in s60_crt_6_exp_srand.txt).
    return (static_cast<float>(rand()) / 32767.0f) <= threshold;
}

} // namespace rng
} // namespace oblivion
