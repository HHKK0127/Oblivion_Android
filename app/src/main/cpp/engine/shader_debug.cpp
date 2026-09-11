#include "shader_debug.h"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <sstream>
#include <vector>

#define LOG_TAG "ShaderDebug"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

ShaderDebug::ShaderInfo ShaderDebug::getShaderInfo(uint32_t programId) {
    ShaderInfo info;
    info.programId = programId;
    info.isValid = false;
    info.compileTimeMs = 0.0f;
    info.useCount = 0;

    if (programId == 0) return info;

    // Check if valid program
    GLint linked = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    info.isValid = (linked == GL_TRUE);

    // Get vertex shader source
    GLint numShaders = 0;
    glGetProgramiv(programId, GL_ATTACHED_SHADERS, &numShaders);
    
    if (numShaders > 0) {
        std::vector<GLuint> shaders(numShaders);
        glGetAttachedShaders(programId, numShaders, nullptr, shaders.data());
        
        for (GLuint shader : shaders) {
            GLint shaderType = 0;
            glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
            
            GLint sourceLen = 0;
            glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &sourceLen);
            
            if (sourceLen > 0) {
                std::string source(sourceLen, '\0');
                glGetShaderSource(shader, sourceLen, nullptr, &source[0]);
                
                if (shaderType == GL_VERTEX_SHADER) {
                    info.vertexSource = source;
                } else if (shaderType == GL_FRAGMENT_SHADER) {
                    info.fragmentSource = source;
                }
            }
        }
    }

    return info;
}

std::vector<ShaderDebug::UniformInfo> ShaderDebug::getUniforms(uint32_t programId) {
    std::vector<UniformInfo> uniforms;
    
    if (programId == 0) return uniforms;

    GLint count = 0;
    glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &count);

    GLint maxNameLen = 0;
    glGetProgramiv(programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLen);

    for (int i = 0; i < count; i++) {
        UniformInfo info;
        std::vector<char> name(maxNameLen);
        GLsizei nameLen = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveUniform(programId, i, maxNameLen, &nameLen, &size, &type, name.data());
        
        info.name = std::string(name.data(), nameLen);
        info.location = glGetUniformLocation(programId, info.name.c_str());
        info.size = size;
        info.type = type;

        uniforms.push_back(info);
    }

    return uniforms;
}

std::vector<ShaderDebug::AttribInfo> ShaderDebug::getAttributes(uint32_t programId) {
    std::vector<AttribInfo> attribs;
    
    if (programId == 0) return attribs;

    GLint count = 0;
    glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTES, &count);

    GLint maxNameLen = 0;
    glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLen);

    for (int i = 0; i < count; i++) {
        AttribInfo info;
        std::vector<char> name(maxNameLen);
        GLsizei nameLen = 0;
        GLint size = 0;
        GLenum type = 0;

        glGetActiveAttrib(programId, i, maxNameLen, &nameLen, &size, &type, name.data());
        
        info.name = std::string(name.data(), nameLen);
        info.location = glGetAttribLocation(programId, info.name.c_str());
        info.size = size;
        info.type = type;

        attribs.push_back(info);
    }

    return attribs;
}

bool ShaderDebug::validateProgram(uint32_t programId) {
    if (programId == 0) return false;

    glValidateProgram(programId);
    
    GLint status = 0;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &status);
    
    if (status == GL_FALSE) {
        LOGI("Program %u validation failed: %s", programId, getProgramLog(programId).c_str());
        return false;
    }
    
    return true;
}

std::string ShaderDebug::getShaderLog(uint32_t shaderId) {
    if (shaderId == 0) return "";

    GLint logLen = 0;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen > 0) {
        std::vector<char> log(logLen);
        glGetShaderInfoLog(shaderId, logLen, nullptr, log.data());
        return std::string(log.data(), logLen);
    }

    return "";
}

std::string ShaderDebug::getProgramLog(uint32_t programId) {
    if (programId == 0) return "";

    GLint logLen = 0;
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen > 0) {
        std::vector<char> log(logLen);
        glGetProgramInfoLog(programId, logLen, nullptr, log.data());
        return std::string(log.data(), logLen);
    }

    return "";
}

void ShaderDebug::dumpShaderSource(uint32_t programId) {
    ShaderInfo info = getShaderInfo(programId);
    
    LOGI("=== Shader Program %u ===", programId);
    LOGI("Valid: %s", info.isValid ? "YES" : "NO");
    
    if (!info.vertexSource.empty()) {
        LOGI("--- Vertex Shader ---");
        // Log in chunks to avoid truncation
        size_t pos = 0;
        int line = 1;
        while (pos < info.vertexSource.size()) {
            size_t end = info.vertexSource.find('\n', pos);
            if (end == std::string::npos) end = info.vertexSource.size();
            std::string lineStr = info.vertexSource.substr(pos, end - pos);
            LOGI("  %3d: %s", line++, lineStr.c_str());
            pos = end + 1;
        }
    }
    
    if (!info.fragmentSource.empty()) {
        LOGI("--- Fragment Shader ---");
        size_t pos = 0;
        int line = 1;
        while (pos < info.fragmentSource.size()) {
            size_t end = info.fragmentSource.find('\n', pos);
            if (end == std::string::npos) end = info.fragmentSource.size();
            std::string lineStr = info.fragmentSource.substr(pos, end - pos);
            LOGI("  %3d: %s", line++, lineStr.c_str());
            pos = end + 1;
        }
    }
}

uint32_t ShaderDebug::getCurrentProgram() {
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    return static_cast<uint32_t>(program);
}

std::string ShaderDebug::formatShaderInfo(uint32_t programId) {
    ShaderInfo info = getShaderInfo(programId);
    
    std::stringstream ss;
    ss << "Program ID: " << programId << "\n";
    ss << "Valid: " << (info.isValid ? "YES" : "NO") << "\n";
    ss << "Vertex lines: " << std::count(info.vertexSource.begin(), info.vertexSource.end(), '\n') << "\n";
    ss << "Fragment lines: " << std::count(info.fragmentSource.begin(), info.fragmentSource.end(), '\n');
    
    return ss.str();
}

static const char* glTypeToString(int type) {
    switch (type) {
        case GL_FLOAT: return "float";
        case GL_FLOAT_VEC2: return "vec2";
        case GL_FLOAT_VEC3: return "vec3";
        case GL_FLOAT_VEC4: return "vec4";
        case GL_INT: return "int";
        case GL_INT_VEC2: return "ivec2";
        case GL_INT_VEC3: return "ivec3";
        case GL_INT_VEC4: return "ivec4";
        case GL_BOOL: return "bool";
        case GL_FLOAT_MAT2: return "mat2";
        case GL_FLOAT_MAT3: return "mat3";
        case GL_FLOAT_MAT4: return "mat4";
        case GL_SAMPLER_2D: return "sampler2D";
        case GL_SAMPLER_CUBE: return "samplerCube";
        default: return "unknown";
    }
}

std::string ShaderDebug::formatUniforms(uint32_t programId) {
    auto uniforms = getUniforms(programId);
    
    std::stringstream ss;
    ss << "Uniforms (" << uniforms.size() << "):\n";
    
    for (const auto& u : uniforms) {
        ss << "  " << glTypeToString(u.type) << " " << u.name;
        if (u.size > 1) ss << "[" << u.size << "]";
        ss << " (loc=" << u.location << ")\n";
    }
    
    return ss.str();
}

std::string ShaderDebug::formatAttributes(uint32_t programId) {
    auto attribs = getAttributes(programId);
    
    std::stringstream ss;
    ss << "Attributes (" << attribs.size() << "):\n";
    
    for (const auto& a : attribs) {
        ss << "  " << glTypeToString(a.type) << " " << a.name;
        if (a.size > 1) ss << "[" << a.size << "]";
        ss << " (loc=" << a.location << ")\n";
    }
    
    return ss.str();
}
