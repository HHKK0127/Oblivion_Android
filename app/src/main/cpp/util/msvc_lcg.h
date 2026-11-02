#pragma once

// MSVC-compatible rand()/srand() LCG stream.
//
// RE source: docs/RE_EXE_REPORT.md section 60.3 and tmp/re/s60_crt.md.
// Oblivion.exe uses the classic MSVC linear congruential generator:
//   state = state * 214013 + 2531011   (uint32 wrap)
//   output = (state >> 16) & 0x7FFF    (RAND_MAX = 32767)
// Seeds flow through srand() at 0x0059D0 and values come back from
// Rand() at 0x0047DF80 over a single process-global stream.
//
// This module replaces the scattered std::mt19937 / std::rand call sites
// (j6-rng-rounding) so gameplay rolls match the original executable.

#include <cstdint>

namespace oblivion {
namespace rng {

// MSVC RAND_MAX for this LCG.
inline constexpr int kRandMax = 32767;

// Seed the global stream (MSVC srand semantics: seed 0 is treated as 1).
void srand(uint32_t seed);

// Next value on the global stream, range [0, 32767] (MSVC rand semantics).
int rand();

// RE 0x0047DF80 Rand() wrapper. snake_case port of the original routine:
//   seed != 0            -> reseed the stream, then return rand()
//   seed == 0, unseeded  -> lazy first-time time(NULL) seed, then rand()
//   seed == 0, seeded    -> just return rand()
// The unseeded flag starts true so a zero seed before any explicit srand()
// only initializes once (matches the original deferred-seed behavior).
int rand_with_seed(uint32_t seed);

// Integer roll in [min, max], inclusive on both ends (same contract as
// std::uniform_int_distribution): min + rand() % (max - min + 1).
// max <= min returns min.
int rand_range(int min, int max);

// Chance roll: true when (rand() / 32767.0f) <= threshold.
// The RE probability helper FUN_0047FB70 uses <= (see s60_crt_6_exp_srand.txt).
bool rand_bool(float threshold);

} // namespace rng
} // namespace oblivion
