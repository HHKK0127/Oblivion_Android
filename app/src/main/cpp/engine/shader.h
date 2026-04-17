#pragma once

#include <string>

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    bool compile(const std::string& vertexSource, const std::string& fragmentSource);
    void use() const;
    void setUniform(const std::string& name, float value);

private:
    unsigned int programId;
};
