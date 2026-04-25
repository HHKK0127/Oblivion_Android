#pragma once

#include <cmath>
#include <array>

namespace glm {

// Vector types
struct vec2 {
    float x, y;
    vec2() : x(0), y(0) {}
    vec2(float x, float y) : x(x), y(y) {}
};

struct vec3 {
    float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }

    float dot(const vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    vec3 cross(const vec3& v) const {
        return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    vec3 normalize() const {
        float len = std::sqrt(x * x + y * y + z * z);
        return vec3(x / len, y / len, z / len);
    }
};

struct vec4 {
    float x, y, z, w;
    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

// Matrix type (4x4)
struct mat4 {
    std::array<std::array<float, 4>, 4> data;

    mat4() {
        // Identity matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                data[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    float* value_ptr() { return &data[0][0]; }
    const float* value_ptr() const { return &data[0][0]; }

    // Subscript operator for accessing rows
    std::array<float, 4>& operator[](int index) { return data[index]; }
    const std::array<float, 4>& operator[](int index) const { return data[index]; }
};

// Utility functions
inline mat4 perspective(float fov, float aspect, float near, float far) {
    mat4 result;
    float f = 1.0f / std::tan(fov / 2.0f);
    result.data[0][0] = f / aspect;
    result.data[1][1] = f;
    result.data[2][2] = (far + near) / (near - far);
    result.data[2][3] = -1.0f;
    result.data[3][2] = (2.0f * far * near) / (near - far);
    result.data[3][3] = 0.0f;
    return result;
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = (center - eye).normalize();
    vec3 s = f.cross(up).normalize();
    vec3 u = s.cross(f);
    
    mat4 result;
    result.data[0][0] = s.x;
    result.data[1][0] = s.y;
    result.data[2][0] = s.z;
    result.data[0][1] = u.x;
    result.data[1][1] = u.y;
    result.data[2][1] = u.z;
    result.data[0][2] = -f.x;
    result.data[1][2] = -f.y;
    result.data[2][2] = -f.z;
    result.data[3][0] = -s.dot(eye);
    result.data[3][1] = -u.dot(eye);
    result.data[3][2] = f.dot(eye);
    return result;
}

inline mat4 rotate(const mat4& m, float angle, const vec3& axis) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;
    
    vec3 a = axis.normalize();
    
    mat4 result;
    result.data[0][0] = t * a.x * a.x + c;
    result.data[0][1] = t * a.x * a.y + a.z * s;
    result.data[0][2] = t * a.x * a.z - a.y * s;
    
    result.data[1][0] = t * a.x * a.y - a.z * s;
    result.data[1][1] = t * a.y * a.y + c;
    result.data[1][2] = t * a.y * a.z + a.x * s;
    
    result.data[2][0] = t * a.x * a.z + a.y * s;
    result.data[2][1] = t * a.y * a.z - a.x * s;
    result.data[2][2] = t * a.z * a.z + c;
    
    return result;
}

inline mat4 translate(const mat4& m, const vec3& v) {
    mat4 result = m;
    result.data[3][0] = v.x;
    result.data[3][1] = v.y;
    result.data[3][2] = v.z;
    return result;
}

inline mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
    mat4 result;
    result.data[0][0] = 2.0f / (right - left);
    result.data[1][1] = 2.0f / (top - bottom);
    result.data[2][2] = -2.0f / (far - near);
    result.data[3][0] = -(right + left) / (right - left);
    result.data[3][1] = -(top + bottom) / (top - bottom);
    result.data[3][2] = -(far + near) / (far - near);
    result.data[3][3] = 1.0f;
    return result;
}

inline float radians(float degrees) {
    return degrees * 3.14159265359f / 180.0f;
}

// Matrix multiplication
inline mat4 operator*(const mat4& a, const mat4& b) {
    mat4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.data[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result.data[i][j] += a.data[k][j] * b.data[i][k];
            }
        }
    }
    return result;
}

} // namespace glm
