#include "debug_draw.h"

#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "DebugDraw", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "DebugDraw", __VA_ARGS__)

// ---------------------------------------------------------------------------
// Shader sources (GLES 3.0)
// ---------------------------------------------------------------------------

static const char* kVS3D = R"glsl(
#version 300 es
precision mediump float;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uMVP;

out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)glsl";

static const char* kFS3D = R"glsl(
#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)glsl";

static const char* kVS2D = R"glsl(
#version 300 es
precision mediump float;
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uProj;

out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)glsl";

static const char* kFS2D = R"glsl(
#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)glsl";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
        LOGE("DebugDraw shader compile error (%s): %s",
             type == GL_VERTEX_SHADER ? "VS" : "FS", buf);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        LOGE("DebugDraw program link error: %s", buf);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

DebugDraw::DebugDraw()
    : enabled_(true)
    , initialized_(false)
    , shaderProgram_(0)
    , vao_(0)
    , vbo_(0)
    , shader2D_(0)
    , vao2D_(0)
    , vbo2D_(0)
    , locMVP_(-1)
    , locColorFrom_(-1)
    , locColorTo_(-1)
    , loc2DProj_(-1) {
}

DebugDraw::~DebugDraw() {
    shutdown();
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

bool DebugDraw::initialize() {
    if (initialized_) return true;

    // Compile 3D shader
    GLuint vs3D = compileShader(GL_VERTEX_SHADER, kVS3D);
    GLuint fs3D = compileShader(GL_FRAGMENT_SHADER, kFS3D);
    if (!vs3D || !fs3D) {
        glDeleteShader(vs3D);
        glDeleteShader(fs3D);
        return false;
    }
    shaderProgram_ = linkProgram(vs3D, fs3D);
    glDeleteShader(vs3D);
    glDeleteShader(fs3D);
    if (!shaderProgram_) return false;

    locMVP_ = glGetUniformLocation(shaderProgram_, "uMVP");

    // Compile 2D shader
    GLuint vs2D = compileShader(GL_VERTEX_SHADER, kVS2D);
    GLuint fs2D = compileShader(GL_FRAGMENT_SHADER, kFS2D);
    if (!vs2D || !fs2D) {
        glDeleteShader(vs2D);
        glDeleteShader(fs2D);
        return false;
    }
    shader2D_ = linkProgram(vs2D, fs2D);
    glDeleteShader(vs2D);
    glDeleteShader(fs2D);
    if (!shader2D_) return false;

    loc2DProj_ = glGetUniformLocation(shader2D_, "uProj");

    // Create VAO / VBO for 3D
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    // Position (vec3) + Color (vec4) = 7 floats per vertex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create VAO / VBO for 2D
    glGenVertexArrays(1, &vao2D_);
    glGenBuffers(1, &vbo2D_);
    glBindVertexArray(vao2D_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo2D_);
    // Position (vec2) + Color (vec4) = 6 floats per vertex
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    initialized_ = true;
    LOGI("DebugDraw initialized (3D prog=%u, 2D prog=%u)", shaderProgram_, shader2D_);
    return true;
}

void DebugDraw::shutdown() {
    if (vao_)  { glDeleteVertexArrays(1, &vao_);  vao_ = 0; }
    if (vbo_)  { glDeleteBuffers(1, &vbo_);        vbo_ = 0; }
    if (vao2D_) { glDeleteVertexArrays(1, &vao2D_); vao2D_ = 0; }
    if (vbo2D_) { glDeleteBuffers(1, &vbo2D_);       vbo2D_ = 0; }
    if (shaderProgram_) { glDeleteProgram(shaderProgram_); shaderProgram_ = 0; }
    if (shader2D_)      { glDeleteProgram(shader2D_);      shader2D_ = 0; }
    lines3D_.clear();
    lines2D_.clear();
    quads2D_.clear();
    vertexBuffer_.clear();
    initialized_ = false;
}

// ---------------------------------------------------------------------------
// 3D Primitives
// ---------------------------------------------------------------------------

void DebugDraw::line(const glm::vec3& from, const glm::vec3& to,
                     const glm::vec4& color, int framesToLive) {
    if (!enabled_) return;
    lines3D_.push_back({from, to, color, color, framesToLive});
}

void DebugDraw::line(const glm::vec3& from, const glm::vec3& to,
                     const glm::vec4& colorFrom, const glm::vec4& colorTo,
                     int framesToLive) {
    if (!enabled_) return;
    lines3D_.push_back({from, to, colorFrom, colorTo, framesToLive});
}

void DebugDraw::cross(const glm::vec3& pos, float size,
                      const glm::vec4& color, int framesToLive) {
    if (!enabled_) return;
    const float h = size * 0.5f;
    line(pos + glm::vec3(-h, 0, 0), pos + glm::vec3(h, 0, 0), color, framesToLive);
    line(pos + glm::vec3(0, -h, 0), pos + glm::vec3(0, h, 0), color, framesToLive);
    line(pos + glm::vec3(0, 0, -h), pos + glm::vec3(0, 0, h), color, framesToLive);
}

void DebugDraw::boxAABB(const glm::vec3& min, const glm::vec3& max,
                        const glm::vec4& color, int framesToLive) {
    if (!enabled_) return;
    const glm::vec3& a = min;
    const glm::vec3& b = max;

    // Bottom face
    line(glm::vec3(a.x, a.y, a.z), glm::vec3(b.x, a.y, a.z), color, framesToLive);
    line(glm::vec3(b.x, a.y, a.z), glm::vec3(b.x, a.y, b.z), color, framesToLive);
    line(glm::vec3(b.x, a.y, b.z), glm::vec3(a.x, a.y, b.z), color, framesToLive);
    line(glm::vec3(a.x, a.y, b.z), glm::vec3(a.x, a.y, a.z), color, framesToLive);
    // Top face
    line(glm::vec3(a.x, b.y, a.z), glm::vec3(b.x, b.y, a.z), color, framesToLive);
    line(glm::vec3(b.x, b.y, a.z), glm::vec3(b.x, b.y, b.z), color, framesToLive);
    line(glm::vec3(b.x, b.y, b.z), glm::vec3(a.x, b.y, b.z), color, framesToLive);
    line(glm::vec3(a.x, b.y, b.z), glm::vec3(a.x, b.y, a.z), color, framesToLive);
    // Vertical edges
    line(glm::vec3(a.x, a.y, a.z), glm::vec3(a.x, b.y, a.z), color, framesToLive);
    line(glm::vec3(b.x, a.y, a.z), glm::vec3(b.x, b.y, a.z), color, framesToLive);
    line(glm::vec3(b.x, a.y, b.z), glm::vec3(b.x, b.y, b.z), color, framesToLive);
    line(glm::vec3(a.x, a.y, b.z), glm::vec3(a.x, b.y, b.z), color, framesToLive);
}

void DebugDraw::boxOBB(const glm::vec3& min, const glm::vec3& max,
                       const glm::mat4& transform, const glm::vec4& color,
                       int framesToLive) {
    if (!enabled_) return;
    // 8 corners of the AABB, then transform
    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z),
    };
    for (int i = 0; i < 8; ++i) {
        glm::vec4 t = transform * glm::vec4(corners[i].x, corners[i].y, corners[i].z, 1.0f);
        corners[i] = glm::vec3(t.x / t.w, t.y / t.w, t.z / t.w);
    }
    // 12 edges
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    for (int i = 0; i < 12; ++i) {
        line(corners[edges[i][0]], corners[edges[i][1]], color, framesToLive);
    }
}

void DebugDraw::sphere(const glm::vec3& center, float radius,
                       const glm::vec4& color, int framesToLive,
                       int segments) {
    if (!enabled_) return;
    buildSphereLines(center, radius, color, framesToLive, segments);
}

void DebugDraw::circle(const glm::vec3& center, float radius,
                       const glm::vec3& normal,
                       const glm::vec4& color, int framesToLive,
                       int segments) {
    if (!enabled_) return;
    buildCircleLines(center, radius, normal, color, framesToLive, segments);
}

void DebugDraw::cylinder(const glm::vec3& start, const glm::vec3& end,
                         float radius, const glm::vec4& color,
                         int framesToLive, int segments) {
    if (!enabled_) return;
    buildCylinderLines(start, end, radius, color, framesToLive, segments);
}

void DebugDraw::cone(const glm::vec3& origin, const glm::vec3& direction,
                     float angleDeg, float length,
                     const glm::vec4& color, int framesToLive,
                     int segments) {
    if (!enabled_) return;
    buildConeLines(origin, direction, angleDeg, length, color, framesToLive, segments);
}

void DebugDraw::grid(const glm::vec3& center, float size, int divisions,
                     const glm::vec4& color, int framesToLive) {
    if (!enabled_) return;
    buildGridLines(center, size, divisions, color, framesToLive);
}

void DebugDraw::arrow(const glm::vec3& from, const glm::vec3& to,
                      const glm::vec4& color, int framesToLive) {
    if (!enabled_) return;
    buildArrowLines(from, to, color, framesToLive);
}

// ---------------------------------------------------------------------------
// 2D Primitives
// ---------------------------------------------------------------------------

void DebugDraw::line2D(const glm::vec2& from, const glm::vec2& to,
                       const glm::vec4& color, int framesToLive) {
    if (!enabled_) return;
    lines2D_.push_back({from, to, color, framesToLive});
}

void DebugDraw::rect2D(const glm::vec2& min, const glm::vec2& max,
                       const glm::vec4& color, bool filled,
                       int framesToLive) {
    if (!enabled_) return;
    quads2D_.push_back({min, max, color, filled, framesToLive});
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DebugDraw::render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix,
                       int screenWidth, int screenHeight) {
    (void)screenWidth;
    (void)screenHeight;
    if (!initialized_) return;

    // Age even when disabled so stale primitives cannot pile up.
    if (!enabled_ || (lines3D_.empty() && lines2D_.empty() && quads2D_.empty())) {
        agePrimitives();
        return;
    }

    const glm::mat4 viewProj = projMatrix * viewMatrix;

    // Save the GL state we touch so the rest of the frame is unaffected.
    const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLint prevProgram = 0;
    GLint prevVao = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);

    // Debug geometry is meant to be readable through world geometry.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Render 3D lines ---
    if (!lines3D_.empty()) {
        rebuild3DBuffer();

        glUseProgram(shaderProgram_);
        glUniformMatrix4fv(locMVP_, 1, GL_FALSE, glm::value_ptr(viewProj));

        glBindVertexArray(vao_);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines3D_.size() * 2));
        glBindVertexArray(0);
    }

    // --- Render 2D primitives ---
    if (!lines2D_.empty() || !quads2D_.empty()) {
        // Orthographic projection: (0,0) = top-left, (1,1) = bottom-right
        const glm::mat4 ortho2D = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f);

        rebuild2DBuffer();

        glUseProgram(shader2D_);
        glUniformMatrix4fv(loc2DProj_, 1, GL_FALSE, glm::value_ptr(ortho2D));

        glBindVertexArray(vao2D_);

        if (!lines2D_.empty()) {
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines2D_.size() * 2));
        }

        // The 2D buffer layout: lines first, then quads (6 verts per filled
        // quad, 4 per outline quad).
        GLint offset = static_cast<GLint>(lines2D_.size() * 2);
        for (const auto& q : quads2D_) {
            if (q.filled) {
                glDrawArrays(GL_TRIANGLES, offset, 6);
                offset += 6;
            } else {
                glDrawArrays(GL_LINE_LOOP, offset, 4);
                offset += 4;
            }
        }

        glBindVertexArray(0);
    }

    // Restore GL state
    glUseProgram(static_cast<GLuint>(prevProgram));
    glBindVertexArray(static_cast<GLuint>(prevVao));
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullWasEnabled)  glEnable(GL_CULL_FACE);  else glDisable(GL_CULL_FACE);
    if (blendWasEnabled) glEnable(GL_BLEND);      else glDisable(GL_BLEND);

    // Age after drawing so a submitted primitive is visible this frame.
    agePrimitives();
}

void DebugDraw::clear() {
    lines3D_.clear();
    lines2D_.clear();
    quads2D_.clear();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void DebugDraw::agePrimitives() {
    // Age 3D lines
    for (auto it = lines3D_.begin(); it != lines3D_.end();) {
        if (it->framesToLive < 0) {
            // Negative = persistent, never age
            ++it;
        } else if (it->framesToLive == 0) {
            it = lines3D_.erase(it);
        } else {
            it->framesToLive--;
            if (it->framesToLive == 0) {
                it = lines3D_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Age 2D lines
    for (auto it = lines2D_.begin(); it != lines2D_.end();) {
        if (it->framesToLive < 0) {
            ++it;
        } else if (it->framesToLive == 0) {
            it = lines2D_.erase(it);
        } else {
            it->framesToLive--;
            if (it->framesToLive == 0) {
                it = lines2D_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Age 2D quads
    for (auto it = quads2D_.begin(); it != quads2D_.end();) {
        if (it->framesToLive < 0) {
            ++it;
        } else if (it->framesToLive == 0) {
            it = quads2D_.erase(it);
        } else {
            it->framesToLive--;
            if (it->framesToLive == 0) {
                it = quads2D_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void DebugDraw::rebuild3DBuffer() {
    // Each DebugLine = 2 vertices, each vertex = 7 floats (pos3 + color4)
    vertexBuffer_.resize(lines3D_.size() * 2 * 7);
    float* ptr = vertexBuffer_.data();
    for (const auto& dl : lines3D_) {
        // Vertex 0 (from)
        ptr[0] = dl.from.x; ptr[1] = dl.from.y; ptr[2] = dl.from.z;
        ptr[3] = dl.colorFrom.x; ptr[4] = dl.colorFrom.y;
                ptr[5] = dl.colorFrom.z; ptr[6] = dl.colorFrom.w;
        ptr += 7;
        // Vertex 1 (to)
        ptr[0] = dl.to.x; ptr[1] = dl.to.y; ptr[2] = dl.to.z;
        ptr[3] = dl.colorTo.x; ptr[4] = dl.colorTo.y;
                ptr[5] = dl.colorTo.z; ptr[6] = dl.colorTo.w;
        ptr += 7;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexBuffer_.size() * sizeof(float),
                 vertexBuffer_.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DebugDraw::rebuild2DBuffer() {
    // Count total vertices: 2 per 2D line, 6 per filled quad, 4 per outline quad
    size_t totalVerts = lines2D_.size() * 2;
    for (const auto& q : quads2D_) {
        totalVerts += q.filled ? 6 : 4;
    }

    vertexBuffer_.resize(totalVerts * 6); // 6 floats per vertex (pos2 + color4)
    float* ptr = vertexBuffer_.data();

    // 2D lines
    for (const auto& dl : lines2D_) {
        ptr[0] = dl.from.x; ptr[1] = dl.from.y;
            ptr[2] = dl.color.x; ptr[3] = dl.color.y;
            ptr[4] = dl.color.z; ptr[5] = dl.color.w;
        ptr += 6;
        ptr[0] = dl.to.x; ptr[1] = dl.to.y;
            ptr[2] = dl.color.x; ptr[3] = dl.color.y;
            ptr[4] = dl.color.z; ptr[5] = dl.color.w;
        ptr += 6;
    }

    // 2D quads
    for (const auto& q : quads2D_) {
        const float r = q.color.x, g = q.color.y, b = q.color.z, a = q.color.w;
        if (q.filled) {
            // Two triangles: (min.x,min.y) -> (max.x,min.y) -> (min.x,max.y)
            //              (max.x,min.y) -> (max.x,max.y) -> (min.x,max.y)
            const float verts[6 * 6] = {
                q.min.x, q.min.y, r, g, b, a,
                q.max.x, q.min.y, r, g, b, a,
                q.min.x, q.max.y, r, g, b, a,
                q.max.x, q.min.y, r, g, b, a,
                q.max.x, q.max.y, r, g, b, a,
                q.min.x, q.max.y, r, g, b, a,
            };
            std::memcpy(ptr, verts, sizeof(verts));
            ptr += 36;
        } else {
            // Outline: 4 corners as line loop
            const float verts[4 * 6] = {
                q.min.x, q.min.y, r, g, b, a,
                q.max.x, q.min.y, r, g, b, a,
                q.max.x, q.max.y, r, g, b, a,
                q.min.x, q.max.y, r, g, b, a,
            };
            std::memcpy(ptr, verts, sizeof(verts));
            ptr += 24;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo2D_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexBuffer_.size() * sizeof(float),
                 vertexBuffer_.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ---------------------------------------------------------------------------
// Build helpers (used by compound primitives)
// ---------------------------------------------------------------------------

void DebugDraw::buildSphereLines(const glm::vec3& center, float radius,
                                 const glm::vec4& color, int framesToLive,
                                 int segments) {
    // Three rings: XY, XZ, YZ
    const float step = 6.28318530718f / segments;

    // XY ring
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * ((i + 1) % segments);
        glm::vec3 p0(center.x + radius * std::cos(a0),
                     center.y + radius * std::sin(a0),
                     center.z);
        glm::vec3 p1(center.x + radius * std::cos(a1),
                     center.y + radius * std::sin(a1),
                     center.z);
        lines3D_.push_back({p0, p1, color, color, framesToLive});
    }
    // XZ ring
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * ((i + 1) % segments);
        glm::vec3 p0(center.x + radius * std::cos(a0),
                     center.y,
                     center.z + radius * std::sin(a0));
        glm::vec3 p1(center.x + radius * std::cos(a1),
                     center.y,
                     center.z + radius * std::sin(a1));
        lines3D_.push_back({p0, p1, color, color, framesToLive});
    }
    // YZ ring
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * ((i + 1) % segments);
        glm::vec3 p0(center.x,
                     center.y + radius * std::cos(a0),
                     center.z + radius * std::sin(a0));
        glm::vec3 p1(center.x,
                     center.y + radius * std::cos(a1),
                     center.z + radius * std::sin(a1));
        lines3D_.push_back({p0, p1, color, color, framesToLive});
    }
}

void DebugDraw::buildCircleLines(const glm::vec3& center, float radius,
                                 const glm::vec3& normal,
                                 const glm::vec4& color, int framesToLive,
                                 int segments) {
    // Build a local basis where normal is up
    glm::vec3 up = glm::normalize(normal);
    glm::vec3 right, forward;
    if (std::abs(up.y) < 0.9f) {
        right = glm::normalize(glm::cross(up, glm::vec3(0, 1, 0)));
    } else {
        right = glm::normalize(glm::cross(up, glm::vec3(1, 0, 0)));
    }
    forward = glm::normalize(glm::cross(right, up));

    const float step = 6.28318530718f / segments;
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * ((i + 1) % segments);
        glm::vec3 p0 = center + (right * std::cos(a0) + forward * std::sin(a0)) * radius;
        glm::vec3 p1 = center + (right * std::cos(a1) + forward * std::sin(a1)) * radius;
        lines3D_.push_back({p0, p1, color, color, framesToLive});
    }
}

void DebugDraw::buildCylinderLines(const glm::vec3& start, const glm::vec3& end,
                                   float radius, const glm::vec4& color,
                                   int framesToLive, int segments) {
    glm::vec3 axis = end - start;
    float len = glm::length(axis);
    if (len < 0.001f) return;
    axis /= len;

    // Build basis
    glm::vec3 right, forward;
    if (std::abs(axis.y) < 0.9f) {
        right = glm::normalize(glm::cross(axis, glm::vec3(0, 1, 0)));
    } else {
        right = glm::normalize(glm::cross(axis, glm::vec3(1, 0, 0)));
    }
    forward = glm::normalize(glm::cross(right, axis));

    const float step = 6.28318530718f / segments;
    // Top and bottom rings
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * ((i + 1) % segments);
        glm::vec3 r0 = (right * std::cos(a0) + forward * std::sin(a0)) * radius;
        glm::vec3 r1 = (right * std::cos(a1) + forward * std::sin(a1)) * radius;

        // Bottom ring segment
        lines3D_.push_back({start + r0, start + r1, color, color, framesToLive});
        // Top ring segment
        lines3D_.push_back({end + r0, end + r1, color, color, framesToLive});
        // Vertical line
        lines3D_.push_back({start + r0, end + r0, color, color, framesToLive});
    }
}

void DebugDraw::buildConeLines(const glm::vec3& origin,
                               const glm::vec3& direction,
                               float angleDeg, float length,
                               const glm::vec4& color, int framesToLive,
                               int segments) {
    glm::vec3 dir = glm::normalize(direction);
    float halfAngle = glm::radians(angleDeg * 0.5f);
    float radius = length * std::tan(halfAngle);

    // Build basis perpendicular to direction
    glm::vec3 right, forward;
    if (std::abs(dir.y) < 0.9f) {
        right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    } else {
        right = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
    }
    forward = glm::normalize(glm::cross(right, dir));

    glm::vec3 tip = origin + dir * length;
    const float step = 6.28318530718f / segments;

    // Base circle
    for (int i = 0; i < segments; ++i) {
        float a0 = step * i;
        float a1 = step * ((i + 1) % segments);
        glm::vec3 p0 = tip + (right * std::cos(a0) + forward * std::sin(a0)) * radius;
        glm::vec3 p1 = tip + (right * std::cos(a1) + forward * std::sin(a1)) * radius;
        lines3D_.push_back({p0, p1, color, color, framesToLive});
        // Line from origin to base point
        lines3D_.push_back({origin, p0, color, color, framesToLive});
    }
}

void DebugDraw::buildGridLines(const glm::vec3& center, float size,
                               int divisions, const glm::vec4& color,
                               int framesToLive) {
    float half = size * 0.5f;
    float step = size / divisions;
    float startX = center.x - half;
    float startZ = center.z - half;

    for (int i = 0; i <= divisions; ++i) {
        float x = startX + i * step;
        float z = startZ + i * step;
        // Line along X
        lines3D_.push_back({glm::vec3(startX, center.y, z),
                            glm::vec3(startX + size, center.y, z),
                            color, color, framesToLive});
        // Line along Z
        lines3D_.push_back({glm::vec3(x, center.y, startZ),
                            glm::vec3(x, center.y, startZ + size),
                            color, color, framesToLive});
    }
}

void DebugDraw::buildArrowLines(const glm::vec3& from, const glm::vec3& to,
                                const glm::vec4& color, int framesToLive) {
    // Main shaft
    lines3D_.push_back({from, to, color, color, framesToLive});

    // Arrow head
    glm::vec3 dir = to - from;
    float len = glm::length(dir);
    if (len < 0.01f) return;
    dir /= len;

    float headLen = std::min(len * 0.25f, 1.0f);
    float headWidth = headLen * 0.4f;

    // Build basis
    glm::vec3 right, up;
    if (std::abs(dir.y) < 0.9f) {
        right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    } else {
        right = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
    }
    up = glm::normalize(glm::cross(right, dir));

    glm::vec3 base = to - dir * headLen;
    glm::vec3 p1 = base + right * headWidth;
    glm::vec3 p2 = base - right * headWidth;
    glm::vec3 p3 = base + up * headWidth;
    glm::vec3 p4 = base - up * headWidth;

    lines3D_.push_back({to, p1, color, color, framesToLive});
    lines3D_.push_back({to, p2, color, color, framesToLive});
    lines3D_.push_back({to, p3, color, color, framesToLive});
    lines3D_.push_back({to, p4, color, color, framesToLive});
}