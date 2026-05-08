#pragma once

#include "../glm.hpp"

namespace glm {
namespace gtc {

inline float* value_ptr(mat4& m) {
    return m.value_ptr();
}

inline const float* value_ptr(const mat4& m) {
    return m.value_ptr();
}

} // namespace gtc

// For direct namespace access
using glm::gtc::value_ptr;

} // namespace glm
