#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Shader debugging utilities
 * 
 * Provides shader inspection, validation, and debugging features.
 */
class ShaderDebug {
public:
    struct ShaderInfo {
        uint32_t programId;
        std::string name;
        std::string vertexSource;
        std::string fragmentSource;
        bool isValid;
        float compileTimeMs;
        uint32_t useCount;
    };

    struct UniformInfo {
        std::string name;
        int location;
        int size;
        int type; // GL_FLOAT, GL_VEC2, GL_VEC3, GL_VEC4, GL_MAT4, etc.
    };

    struct AttribInfo {
        std::string name;
        int location;
        int size;
        int type;
    };

    // Get info about a specific shader program
    static ShaderInfo getShaderInfo(uint32_t programId);

    // Get all uniforms for a program
    static std::vector<UniformInfo> getUniforms(uint32_t programId);

    // Get all attributes for a program
    static std::vector<AttribInfo> getAttributes(uint32_t programId);

    // Validate a shader program
    static bool validateProgram(uint32_t programId);

    // Get shader compilation error log
    static std::string getShaderLog(uint32_t shaderId);

    // Get program link error log
    static std::string getProgramLog(uint32_t programId);

    // Dump shader source to log
    static void dumpShaderSource(uint32_t programId);

    // Get current active shader program
    static uint32_t getCurrentProgram();

    // Format shader info as string for display
    static std::string formatShaderInfo(uint32_t programId);
    static std::string formatUniforms(uint32_t programId);
    static std::string formatAttributes(uint32_t programId);
};
