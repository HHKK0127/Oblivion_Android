#include "shader.h"

ShaderProgram::ShaderProgram() : programId(0) {}

ShaderProgram::~ShaderProgram() {}

bool ShaderProgram::compile(const std::string& vertexSource,
                            const std::string& fragmentSource) {
    return true;
}

void ShaderProgram::use() const {}

void ShaderProgram::setUniform(const std::string& name, float value) {}
