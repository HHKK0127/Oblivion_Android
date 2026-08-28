#pragma once

#include <cmath>
#include <array>

namespace glm {

// Vector types
struct vec2 {
    float x, y;
    vec2() : x(0), y(0) {}
    vec2(float x, float y) : x(x), y(y) {}
    vec2(float s) : x(s), y(s) {}

    vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
    vec2 operator*(float s) const { return vec2(x * s, y * s); }
    vec2 operator/(float s) const { return vec2(x / s, y / s); }
    vec2& operator+=(const vec2& v) { x += v.x; y += v.y; return *this; }
    vec2& operator-=(const vec2& v) { x -= v.x; y -= v.y; return *this; }
    vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    vec2 normalize() const { float len = length(); return vec2(x / len, y / len); }
};

struct vec3 {
    float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    vec3 operator/(float s) const { return vec3(x / s, y / s, z / s); }
    vec3& operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

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
    vec4(float s) : x(s), y(s), z(s), w(s) {}

    vec4 operator+(const vec4& v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator-(const vec4& v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }
    vec4& operator+=(const vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
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

// Matrix * vector
inline vec4 operator*(const mat4& m, const vec4& v) {
    return vec4(
        m.data[0][0]*v.x + m.data[1][0]*v.y + m.data[2][0]*v.z + m.data[3][0]*v.w,
        m.data[0][1]*v.x + m.data[1][1]*v.y + m.data[2][1]*v.z + m.data[3][1]*v.w,
        m.data[0][2]*v.x + m.data[1][2]*v.y + m.data[2][2]*v.z + m.data[3][2]*v.w,
        m.data[0][3]*v.x + m.data[1][3]*v.y + m.data[2][3]*v.z + m.data[3][3]*v.w
    );
}

// 4x4 matrix inverse (Gauss-Jordan)
inline mat4 inverse(const mat4& m) {
    mat4 inv;
    float det;
    const float* v = &m.data[0][0];
    float* o = &inv.data[0][0];

    o[0]  =  v[5]*v[10]*v[15] - v[5]*v[11]*v[14] - v[9]*v[6]*v[15] + v[9]*v[7]*v[14] + v[13]*v[6]*v[11] - v[13]*v[7]*v[10];
    o[4]  = -v[4]*v[10]*v[15] + v[4]*v[11]*v[14] + v[8]*v[6]*v[15] - v[8]*v[7]*v[14] - v[12]*v[6]*v[11] + v[12]*v[7]*v[10];
    o[8]  =  v[4]*v[9]*v[15]  - v[4]*v[11]*v[13] - v[8]*v[5]*v[15] + v[8]*v[7]*v[13] + v[12]*v[5]*v[11] - v[12]*v[7]*v[9];
    o[12] = -v[4]*v[9]*v[14]  + v[4]*v[10]*v[13] + v[8]*v[5]*v[14] - v[8]*v[6]*v[13] - v[12]*v[5]*v[10] + v[12]*v[6]*v[9];
    o[1]  = -v[1]*v[10]*v[15] + v[1]*v[11]*v[14] + v[9]*v[2]*v[15] - v[9]*v[3]*v[14] - v[13]*v[2]*v[11] + v[13]*v[3]*v[10];
    o[5]  =  v[0]*v[10]*v[15] - v[0]*v[11]*v[14] - v[8]*v[2]*v[15] + v[8]*v[3]*v[14] + v[12]*v[2]*v[11] - v[12]*v[3]*v[10];
    o[9]  = -v[0]*v[9]*v[15]  + v[0]*v[11]*v[13] + v[8]*v[1]*v[15] - v[8]*v[3]*v[13] - v[12]*v[1]*v[11] + v[12]*v[3]*v[9];
    o[13] =  v[0]*v[9]*v[14]  - v[0]*v[10]*v[13] - v[8]*v[1]*v[14] + v[8]*v[2]*v[13] + v[12]*v[1]*v[10] - v[12]*v[2]*v[9];
    o[2]  =  v[1]*v[6]*v[15]  - v[1]*v[7]*v[14]  - v[5]*v[2]*v[15] + v[5]*v[3]*v[14] + v[13]*v[2]*v[7]  - v[13]*v[3]*v[6];
    o[6]  = -v[0]*v[6]*v[15]  + v[0]*v[7]*v[14]  + v[4]*v[2]*v[15] - v[4]*v[3]*v[14] - v[12]*v[2]*v[7]  + v[12]*v[3]*v[6];
    o[10] =  v[0]*v[5]*v[15]  - v[0]*v[7]*v[13]  - v[4]*v[1]*v[15] + v[4]*v[3]*v[13] + v[12]*v[1]*v[7]  - v[12]*v[3]*v[5];
    o[14] = -v[0]*v[5]*v[14]  + v[0]*v[6]*v[13]  + v[4]*v[1]*v[14] - v[4]*v[2]*v[13] - v[12]*v[1]*v[6]  + v[12]*v[2]*v[5];
    o[3]  = -v[1]*v[6]*v[11]  + v[1]*v[7]*v[10]  + v[5]*v[2]*v[11] - v[5]*v[3]*v[10] - v[9]*v[2]*v[7]   + v[9]*v[3]*v[6];
    o[7]  =  v[0]*v[6]*v[11]  - v[0]*v[7]*v[10]  - v[4]*v[2]*v[11] + v[4]*v[3]*v[10] + v[8]*v[2]*v[7]   - v[8]*v[3]*v[6];
    o[11] = -v[0]*v[5]*v[11]  + v[0]*v[7]*v[9]   + v[4]*v[1]*v[11] - v[4]*v[3]*v[9]  - v[8]*v[1]*v[7]   + v[8]*v[3]*v[5];
    o[15] =  v[0]*v[5]*v[10]  - v[0]*v[6]*v[9]   - v[4]*v[1]*v[10] + v[4]*v[2]*v[9]  + v[8]*v[1]*v[6]   - v[8]*v[2]*v[5];

    det = v[0]*o[0] + v[1]*o[4] + v[2]*o[8] + v[3]*o[12];
    if (det == 0.0f) return mat4(); // identity fallback
    det = 1.0f / det;
    for (int i = 0; i < 16; i++) o[i] *= det;
    return inv;
}

// Free functions for vec3
inline vec3 cross(const vec3& a, const vec3& b) { return a.cross(b); }
inline vec3 normalize(const vec3& v) { return v.normalize(); }
inline float dot(const vec3& a, const vec3& b) { return a.dot(b); }
inline float length(const vec3& v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

// Quaternion (Hamilton convention)
struct quat {
    float x, y, z, w;

    quat() : x(0), y(0), z(0), w(1) {}
    quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    quat operator*(const quat& q) const {
        return quat(
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w,
            w*q.w - x*q.x - y*q.y - z*q.z
        );
    }

    quat conjugate() const { return quat(-x, -y, -z, w); }

    // Optimized vector rotation: v' = q * (v,0) * q^-1
    vec3 rotate(const vec3& v) const {
        vec3 u(x, y, z);
        vec3 t = cross(u, v) * 2.0f;
        return v + t * w + cross(u, t);
    }
};

inline float dot(const quat& a, const quat& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

inline quat normalize(const quat& q) {
    float len = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (len < 1e-8f) return quat();
    return quat(q.x/len, q.y/len, q.z/len, q.w/len);
}

// Spherical Linear Interpolation
inline quat slerp(const quat& a, const quat& b, float t) {
    float d = dot(a, b);
    quat b2 = b;
    if (d < 0.0f) { b2 = quat(-b.x, -b.y, -b.z, -b.w); d = -d; }
    if (d > 0.9995f) {
        // Nearly parallel: linear interpolation
        quat r(a.x + t*(b2.x - a.x), a.y + t*(b2.y - a.y),
               a.z + t*(b2.z - a.z), a.w + t*(b2.w - a.w));
        return normalize(r);
    }
    float theta0 = std::acos(d);
    float theta = theta0 * t;
    float sinTheta = std::sin(theta);
    float sinTheta0 = std::sin(theta0);
    float s0 = std::cos(theta) - d * sinTheta / sinTheta0;
    float s1 = sinTheta / sinTheta0;
    return quat(a.x*s0 + b2.x*s1, a.y*s0 + b2.y*s1,
                a.z*s0 + b2.z*s1, a.w*s0 + b2.w*s1);
}

// Convert quaternion to mat4 (rotation matrix)
inline mat4 quatToMat4(const quat& q) {
    float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
    float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
    float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;

    mat4 m;
    m.data[0][0] = 1.0f - 2.0f*(yy + zz);
    m.data[0][1] = 2.0f*(xy + wz);
    m.data[0][2] = 2.0f*(xz - wy);
    m.data[0][3] = 0.0f;
    m.data[1][0] = 2.0f*(xy - wz);
    m.data[1][1] = 1.0f - 2.0f*(xx + zz);
    m.data[1][2] = 2.0f*(yz + wx);
    m.data[1][3] = 0.0f;
    m.data[2][0] = 2.0f*(xz + wy);
    m.data[2][1] = 2.0f*(yz - wx);
    m.data[2][2] = 1.0f - 2.0f*(xx + yy);
    m.data[2][3] = 0.0f;
    m.data[3][0] = 0.0f;
    m.data[3][1] = 0.0f;
    m.data[3][2] = 0.0f;
    m.data[3][3] = 1.0f;
    return m;
}

// Build TRS matrix: Translation * Rotation * Scale
inline mat4 trsMatrix(const vec3& translation, const quat& rotation, float scale) {
    mat4 r = quatToMat4(rotation);
    // Apply scale
    for (int c = 0; c < 3; c++) {
        for (int row = 0; row < 3; row++) {
            r.data[c][row] *= scale;
        }
    }
    // Set translation
    r.data[3][0] = translation.x;
    r.data[3][1] = translation.y;
    r.data[3][2] = translation.z;
    return r;
}

} // namespace glm
