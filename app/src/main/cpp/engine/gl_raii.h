#pragma once

// ============================================================================
// GL RAII Wrappers
// Automatic cleanup of OpenGL objects (VAO, VBO, Texture, Shader Program)
// ============================================================================

#include <GLES3/gl3.h>
#include <utility>
#include <string>
#include <android/log.h>

#define LOG_TAG_GLRAII "GLRAII"

namespace glr {

// VAO wrapper
struct VAO {
    GLuint id = 0;
    VAO() { glGenVertexArrays(1, &id); }
    ~VAO() { if (id) glDeleteVertexArrays(1, &id); }
    VAO(const VAO&) = delete;
    VAO& operator=(const VAO&) = delete;
    VAO(VAO&& o) noexcept : id(std::exchange(o.id, 0)) {}
    VAO& operator=(VAO&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteVertexArrays(1, &id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }
    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
};

// VBO wrapper
struct VBO {
    GLuint id = 0;
    VBO() { glGenBuffers(1, &id); }
    ~VBO() { if (id) glDeleteBuffers(1, &id); }
    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;
    VBO(VBO&& o) noexcept : id(std::exchange(o.id, 0)) {}
    VBO& operator=(VBO&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteBuffers(1, &id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }
    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
};

// EBO (Element Buffer Object) wrapper
struct EBO {
    GLuint id = 0;
    EBO() { glGenBuffers(1, &id); }
    ~EBO() { if (id) glDeleteBuffers(1, &id); }
    EBO(const EBO&) = delete;
    EBO& operator=(const EBO&) = delete;
    EBO(EBO&& o) noexcept : id(std::exchange(o.id, 0)) {}
    EBO& operator=(EBO&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteBuffers(1, &id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }
    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
};

// Texture wrapper
struct Tex2D {
    GLuint id = 0;
    Tex2D() { glGenTextures(1, &id); }
    ~Tex2D() { if (id) glDeleteTextures(1, &id); }
    Tex2D(const Tex2D&) = delete;
    Tex2D& operator=(const Tex2D&) = delete;
    Tex2D(Tex2D&& o) noexcept : id(std::exchange(o.id, 0)) {}
    Tex2D& operator=(Tex2D&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteTextures(1, &id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }
    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
};

// FBO (Framebuffer Object) wrapper
struct FBO {
    GLuint id = 0;
    FBO() { glGenFramebuffers(1, &id); }
    ~FBO() { if (id) glDeleteFramebuffers(1, &id); }
    FBO(const FBO&) = delete;
    FBO& operator=(const FBO&) = delete;
    FBO(FBO&& o) noexcept : id(std::exchange(o.id, 0)) {}
    FBO& operator=(FBO&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteFramebuffers(1, &id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }
    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
};

// RBO (Renderbuffer Object) wrapper
struct RBO {
    GLuint id = 0;
    RBO() { glGenRenderbuffers(1, &id); }
    ~RBO() { if (id) glDeleteRenderbuffers(1, &id); }
    RBO(const RBO&) = delete;
    RBO& operator=(const RBO&) = delete;
    RBO(RBO&& o) noexcept : id(std::exchange(o.id, 0)) {}
    RBO& operator=(RBO&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteRenderbuffers(1, &id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }
    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
};

// Shader Program wrapper
struct Prog {
    GLuint id = 0;
    Prog() = default;
    ~Prog() { if (id) glDeleteProgram(id); }
    Prog(const Prog&) = delete;
    Prog& operator=(const Prog&) = delete;
    Prog(Prog&& o) noexcept : id(std::exchange(o.id, 0)) {}
    Prog& operator=(Prog&& o) noexcept {
        if (this != &o) {
            if (id) glDeleteProgram(id);
            id = std::exchange(o.id, 0);
        }
        return *this;
    }

    bool compile(const std::string& vertSrc, const std::string& fragSrc,
                 const std::string& debugName = "") {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        const char* vSrc = vertSrc.c_str();
        glShaderSource(vs, 1, &vSrc, nullptr);
        glCompileShader(vs);
        GLint ok = 0;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GLRAII,
                "Vertex shader '%s' compile error: %s", debugName.c_str(), log);
            glDeleteShader(vs);
            return false;
        }

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fSrc = fragSrc.c_str();
        glShaderSource(fs, 1, &fSrc, nullptr);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GLRAII,
                "Fragment shader '%s' compile error: %s", debugName.c_str(), log);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return false;
        }

        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(id, sizeof(log), nullptr, log);
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GLRAII,
                "Shader program '%s' link error: %s", debugName.c_str(), log);
            glDeleteProgram(id);
            id = 0;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        return id != 0;
    }

    GLuint get() const { return id; }
    explicit operator bool() const { return id != 0; }
    void use() const { if (id) glUseProgram(id); }
};

} // namespace glr
