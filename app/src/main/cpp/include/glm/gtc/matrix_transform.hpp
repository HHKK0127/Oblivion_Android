#pragma once

#include "../glm.hpp"

namespace glm {
namespace gtc {

inline mat4 perspective(float fov, float aspect, float near, float far) {
    return glm::perspective(fov, aspect, near, far);
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    return glm::lookAt(eye, center, up);
}

} // namespace gtc
} // namespace glm
