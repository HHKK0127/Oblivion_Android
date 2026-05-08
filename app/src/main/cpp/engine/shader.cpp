#include "shader.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "ShaderProgram"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

ShaderProgram::ShaderProgram() : programId(0) {}

ShaderProgram::~ShaderProgram() {
    cleanup();
}

bool ShaderProgram::compile(const std::string& vertexSource,
                            const std::string& fragmentSource) {
    LOGD("=== Starting shader compilation ===");

    unsigned int vertexShader = 0;
    unsigned int fragmentShader = 0;

    // Compile vertex shader
    if (!compileShader(vertexShader, GL_VERTEX_SHADER, vertexSource)) {
        LOGE("Failed to compile vertex shader");
        return false;
    }

    // Compile fragment shader
    if (!compileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentSource)) {
        LOGE("Failed to compile fragment shader");
        glDeleteShader(vertexShader);
        return false;
    }

    // Create program and link
    programId = glCreateProgram();
    LOGD("Created shader program: %u", programId);

    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);

    glLinkProgram(programId);

    // Check linking status
    int success;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);

    if (!success) {
        std::string log = getProgramLog();
        LOGE("Shader program linking failed: %s", log.c_str());
        glDeleteProgram(programId);
        programId = 0;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    LOGD("Shader program linked successfully: %u", programId);

    // Clean up shaders (they're linked into program now)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

bool ShaderProgram::compileShader(unsigned int& shader, unsigned int shaderType, const std::string& source) {
    const char* shaderSource = source.c_str();

    shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    // Check compilation status
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        std::string log = getShaderLog(shader);
        const char* shaderType_str = (shaderType == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        LOGE("%s shader compilation failed: %s", shaderType_str, log.c_str());
        glDeleteShader(shader);
        return false;
    }

    return true;
}

std::string ShaderProgram::getShaderLog(unsigned int shader) {
    int maxLength = 512;
    char infoLog[512];
    glGetShaderInfoLog(shader, maxLength, nullptr, infoLog);
    return std::string(infoLog);
}

std::string ShaderProgram::getProgramLog() {
    int maxLength = 512;
    char infoLog[512];
    glGetProgramInfoLog(programId, maxLength, nullptr, infoLog);
    return std::string(infoLog);
}

void ShaderProgram::use() const {
    if (programId != 0) {
        glUseProgram(programId);
    }
}

void ShaderProgram::setUniform(const std::string& name, float value) {
    if (programId == 0) return;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    if (loc != -1) {
        glUniform1f(loc, value);
    }
}

void ShaderProgram::setUniform(const std::string& name, int value) {
    if (programId == 0) return;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    if (loc != -1) {
        glUniform1i(loc, value);
    }
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec2& value) {
    if (programId == 0) return;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    if (loc != -1) {
        glUniform2fv(loc, 1, &value.x);
    }
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec3& value) {
    if (programId == 0) return;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    if (loc != -1) {
        glUniform3fv(loc, 1, &value.x);
    }
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec4& value) {
    if (programId == 0) return;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    if (loc != -1) {
        glUniform4fv(loc, 1, &value.x);
    }
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat4& value) {
    if (programId == 0) return;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    if (loc != -1) {
        // mat4 data is stored as a 4x4 float array in column-major order
        glUniformMatrix4fv(loc, 1, GL_FALSE, reinterpret_cast<const float*>(&value));
    }
}

void ShaderProgram::cleanup() {
    if (programId != 0) {
        glDeleteProgram(programId);
        programId = 0;
    }
}
