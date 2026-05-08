#pragma once

#include <string>
#include <glm/glm.hpp>

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    // Shader compilation and linking
    bool compile(const std::string& vertexSource, const std::string& fragmentSource);

    // Use this shader program
    void use() const;

    // Uniform setters
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, const glm::vec2& value);
    void setUniform(const std::string& name, const glm::vec3& value);
    void setUniform(const std::string& name, const glm::vec4& value);
    void setUniform(const std::string& name, const glm::mat4& value);

    // Getters
    unsigned int getProgramId() const { return programId; }
    bool isValid() const { return programId != 0; }

    // Cleanup
    void cleanup();

private:
    unsigned int programId;

    // Helper methods
    bool compileShader(unsigned int& shader, unsigned int shaderType, const std::string& source);
    std::string getShaderLog(unsigned int shader);
    std::string getProgramLog();
};
