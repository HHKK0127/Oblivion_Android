// No-op host stand-ins for the GLES3 entry points declared in stubs/GLES3/gl3.h.
//
// Suites wired into this harness exercise gameplay, save, world and asset code paths.
// Some of those translation units also contain GPU upload helpers (VAO/VBO/texture
// creation) which must link even though no host test asserts on rendering behaviour.
// These definitions keep the link honest - the real renderer is never built here.
#include <GLES3/gl3.h>

extern "C" {

void glBindBuffer(GLenum, GLuint) {}
void glBindBufferBase(GLenum, GLuint, GLuint) {}
void glBindTexture(GLenum, GLuint) {}
void glBindVertexArray(GLuint) {}
void glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {}
void glBufferSubData(GLenum, GLintptr, GLsizeiptr, const void*) {}
void glDeleteBuffers(GLsizei, const GLuint*) {}
void glDeleteTextures(GLsizei, const GLuint*) {}
void glDeleteVertexArrays(GLsizei, const GLuint*) {}
void glDrawElements(GLenum, GLsizei, GLenum, const void*) {}
void glEnableVertexAttribArray(GLuint) {}
void glGenBuffers(GLsizei, GLuint*) {}
void glGenTextures(GLsizei, GLuint*) {}
void glGenVertexArrays(GLsizei, GLuint*) {}
void glGenerateMipmap(GLenum) {}
void glGetIntegerv(GLenum, GLint*) {}
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
void glTexParameteri(GLenum, GLenum, GLint) {}
void glVertexAttribIPointer(GLuint, GLint, GLenum, GLsizei, const void*) {}
void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) {}

// Shader / program entry points. ShaderProgram never reaches a GPU context here, so
// glCreateShader/glCreateProgram return 0 and the object reports "not compiled".
GLuint glCreateProgram(void) { return 0; }
GLuint glCreateShader(GLenum) { return 0; }
void glAttachShader(GLuint, GLuint) {}
void glCompileShader(GLuint) {}
void glDeleteProgram(GLuint) {}
void glDeleteShader(GLuint) {}
void glGetProgramInfoLog(GLuint, GLsizei, GLsizei*, GLchar*) {}
void glGetProgramiv(GLuint, GLenum, GLint* params) {
  if (params) *params = 0;
}
void glGetShaderInfoLog(GLuint, GLsizei, GLsizei*, GLchar*) {}
void glGetShaderiv(GLuint, GLenum, GLint* params) {
  if (params) *params = 0;
}
GLint glGetUniformLocation(GLuint, const GLchar*) { return -1; }
void glLinkProgram(GLuint) {}
void glShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*) {}
void glUniform1f(GLint, GLfloat) {}
void glUniform1i(GLint, GLint) {}
void glUniform2fv(GLint, GLsizei, const GLfloat*) {}
void glUniform3fv(GLint, GLsizei, const GLfloat*) {}
void glUniform4fv(GLint, GLsizei, const GLfloat*) {}
void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat*) {}
void glUseProgram(GLuint) {}

}  // extern "C"
