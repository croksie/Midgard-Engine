#pragma once
#include <string>
#include <cstdint>

#include <glad/glad.h>

#include "utils/log.h"

#include "bifrost/shader.h"

namespace midgard::bifrost::opengl {

inline GLenum shaderTypeToGLType(ShaderType type) {
    switch (type) {
        case ShaderType::VERTEX: return GL_VERTEX_SHADER;
        case ShaderType::FRAGMENT: return GL_FRAGMENT_SHADER;
        default: 
            ENGINE_LOG_ERROR("Invalid shader type");
            return GL_NONE;
    }
}

class OpenGLShader : public Shader {
public:
    OpenGLShader(ShaderType type, const std::string& source, const std::string& entryPoint = "main");
    ~OpenGLShader() override;

    std::string compileShader(ShaderType type, const std::string& source, const std::string& entryPoint = "main");

    GLuint getShaderID() const { return m_shaderID; }

private:
    GLuint m_shaderID = 0;
};

} // namespace midgard::bifrost::opengl