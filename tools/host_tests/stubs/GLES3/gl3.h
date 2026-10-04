#pragma once
// Host-side stub for GLES3 headers (compile-time only; no symbols used in tests).
#include <cstdint>
#define GL_ES_VERSION_3_0 1
typedef uint32_t GLenum;
typedef uint32_t GLbitfield;
typedef uint32_t GLuint;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef int32_t GLfixed;
typedef uint8_t GLboolean;
typedef float GLfloat;
typedef float GLclampf;
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef struct __GLsync* GLsync;
typedef intptr_t GLintptr;
typedef intptr_t GLsizeiptr;
typedef char GLchar;
typedef void GLvoid;

// Enumerants referenced by the sources compiled for the host suites.
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_TRIANGLES 0x0004
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

// Function entry points are provided as no-ops by tools/host_tests/host_gl_stubs.cpp.
// They exist so that non-rendering suites can link sources that upload GPU buffers
// or compile shader programs; no host test asserts on rendering behaviour.
extern "C" {
void glBindBuffer(GLenum target, GLuint buffer);
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
void glBindTexture(GLenum target, GLuint texture);
void glBindVertexArray(GLuint array);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
void glDeleteBuffers(GLsizei n, const GLuint* buffers);
void glDeleteTextures(GLsizei n, const GLuint* textures);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
void glEnableVertexAttribArray(GLuint index);
void glGenBuffers(GLsizei n, GLuint* buffers);
void glGenTextures(GLsizei n, GLuint* textures);
void glGenVertexArrays(GLsizei n, GLuint* arrays);
void glGenerateMipmap(GLenum target);
void glGetIntegerv(GLenum pname, GLint* data);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width,
                  GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride,
                            const void* pointer);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
                           GLsizei stride, const void* pointer);

// Shader / program entry points (engine/shader.cpp).
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum type);
void glAttachShader(GLuint program, GLuint shader);
void glCompileShader(GLuint shader);
void glDeleteProgram(GLuint program);
void glDeleteShader(GLuint shader);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
GLint glGetUniformLocation(GLuint program, const GLchar* name);
void glLinkProgram(GLuint program);
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string,
                    const GLint* length);
void glUniform1f(GLint location, GLfloat v0);
void glUniform1i(GLint location, GLint v0);
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat* value);
void glUseProgram(GLuint program);
}
