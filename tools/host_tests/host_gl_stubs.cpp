// No-op host stand-ins for the GLES3 entry points declared in stubs/GLES3/gl3.h.
//
// Suites wired into this harness exercise gameplay, save, world and asset code paths.
// Some of those translation units also contain GPU upload helpers (VAO/VBO/texture
// creation) which must link even though no host test asserts on rendering behaviour.
// These definitions keep the link honest - the real renderer is never built here.
#include <GLES3/gl3.h>

extern "C" {

void glBindBuffer(GLenum, GLuint) {}
void glBindTexture(GLenum, GLuint) {}
void glBindVertexArray(GLuint) {}
void glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {}
void glDeleteBuffers(GLsizei, const GLuint*) {}
void glDeleteTextures(GLsizei, const GLuint*) {}
void glDeleteVertexArrays(GLsizei, const GLuint*) {}
void glEnableVertexAttribArray(GLuint) {}
void glGenBuffers(GLsizei, GLuint*) {}
void glGenTextures(GLsizei, GLuint*) {}
void glGenVertexArrays(GLsizei, GLuint*) {}
void glGenerateMipmap(GLenum) {}
void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
void glTexParameteri(GLenum, GLenum, GLint) {}
void glVertexAttribIPointer(GLuint, GLint, GLenum, GLsizei, const void*) {}
void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) {}

}  // extern "C"
