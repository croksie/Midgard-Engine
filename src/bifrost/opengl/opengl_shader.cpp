#include "bifrost/opengl/opengl_shader.h"

#include "utils/log.h"
#include "utils/shader_compiler.h"

namespace midgard::bifrost::opengl {

OpenGLShader::OpenGLShader(ShaderType type, const std::string &source, const std::string& entryPoint) {
    ENGINE_LOG_TRACE("Creating shader...");
    m_shaderID = glCreateShader(shaderTypeToGLType(type));

    std::string glslSource = compileShader(type, source, entryPoint);
    const GLchar *shaderSource = glslSource.c_str();
    glShaderSource(m_shaderID, 1, &shaderSource, NULL);
    glCompileShader(m_shaderID);

    GLint success = 0;
    glGetShaderiv(m_shaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(m_shaderID, sizeof(infoLog), NULL, infoLog);
        ENGINE_LOG_ERROR("Shader compilation failed:\n{}", infoLog);
    } else {
        ENGINE_LOG_TRACE("Shader created");
    }
}

OpenGLShader::~OpenGLShader() {
    if (m_shaderID) {
        glDeleteShader(m_shaderID);
    }
}

std::string OpenGLShader::compileShader(ShaderType type, const std::string &source, const std::string& entryPoint) {
    if (type == ShaderType::VERTEX) {
        std::vector<uint32_t> spirvVertexShader =
            utils::shader::compileHlslToSpirv(source, entryPoint, "vs_6_0");
        return utils::shader::transpileSpirvToGlsl(spirvVertexShader);
    } else if (type == ShaderType::FRAGMENT) {
        std::vector<uint32_t> spirvFragmentShader =
            utils::shader::compileHlslToSpirv(source, entryPoint, "ps_6_0");
        return utils::shader::transpileSpirvToGlsl(spirvFragmentShader);
    }
    return "";
}

} // namespace midgard::bifrost::opengl
