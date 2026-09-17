#pragma once

// ============================================================================
// GL Error Checking Macro
// Debug build only - checks glGetError() after each GL call
// ============================================================================

#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG_TAG_GLCHECK "GLCheck"

inline const char* glCheckErrorString(GLenum err) {
    switch (err) {
        case GL_NO_ERROR:                      return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default:                               return "UNKNOWN_GL_ERROR";
    }
}

#ifdef NDEBUG
// Release build: no-op
#define GLCHECK() ((void)0)
#define GLCHECK_MSG(msg) ((void)0)
#else
// Debug build: check and log
#define GLCHECK() do { \
    GLenum _glErr = glGetError(); \
    if (_glErr != GL_NO_ERROR) { \
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GLCHECK, \
            "%s:%d GL error: %s (0x%04X)", __FILE__, __LINE__, \
            glCheckErrorString(_glErr), _glErr); \
    } \
} while(0)

#define GLCHECK_MSG(msg) do { \
    GLenum _glErr = glGetError(); \
    if (_glErr != GL_NO_ERROR) { \
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_GLCHECK, \
            "%s:%d [%s] GL error: %s (0x%04X)", __FILE__, __LINE__, \
            msg, glCheckErrorString(_glErr), _glErr); \
    } \
} while(0)
#endif
