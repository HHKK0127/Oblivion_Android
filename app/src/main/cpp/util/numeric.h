#pragma once

// Numeric helpers shared across the engine.

#include <cmath>

namespace oblivion {

// Round half-up: -2.5 -> -2, 2.5 -> 3, -0.4 -> 0.
// The original engine rounds through a floor(v + 0.5) sequence, which differs
// from std::round for negative halves (-2.5 -> -2 here, -3 with std::round).
// Use this helper wherever gameplay-visible rounding must match the original.
inline float round_half_up(float x) {
    return std::floorf(x + 0.5f);
}

} // namespace oblivion
