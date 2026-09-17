#include "bifrost/opengl/opengl_pipeline.h"

#include "utils/log.h"

#include "bifrost/bifrost.h"
#include "bifrost/opengl/opengl_buffer.h"
#include "bifrost/opengl/opengl_shader.h"
#include "bifrost/opengl/opengl_texture.h"

namespace midgard::bifrost::opengl {

OpenGLPipeline::OpenGLPipeline(PipelineInfo& info) {
    glCreateVertexArrays(1, &m_vertexArrayID);

    GLuint vertexShader = static_cast<OpenGLShader*>(info.vertexShader.get())->getShaderID();
    GLuint fragmentShader = static_cast<OpenGLShader*>(info.fragmentShader.get())->getShaderID();

    m_shaderProgamID = glCreateProgram();
    glAttachShader(m_shaderProgamID, vertexShader);
    glAttachShader(m_shaderProgamID, fragmentShader);
    glLinkProgram(m_shaderProgamID);

    GLint success;
    glGetProgramiv(m_shaderProgamID, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(m_shaderProgamID, 512, NULL, infoLog);
        ENGINE_LOG_ERROR("OpenGL Program linking failed: {}", infoLog);
    }
}

OpenGLPipeline::~OpenGLPipeline() {
    if (m_shaderProgamID) {
        glDeleteProgram(m_shaderProgamID);
    }
    if (m_vertexArrayID) {
        glDeleteVertexArrays(1, &m_vertexArrayID);
    }
}

void OpenGLPipeline::bindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer) {
    ENGINE_LOG_TRACE("Vertex buffer binding...");
    OpenGLBuffer* buffer = static_cast<OpenGLBuffer*>(vertexBuffer.get());

    if (buffer->getType() != BufferType::VERTEX) {
        ENGINE_LOG_ERROR("Buffer is not of type VERTEX");
        return;
    }

    GLuint attribPos = 0;
    GLuint attribCol = 1;
    GLuint attribTexCoord = 2;

    GLuint vaoBindingPoint = 0;
    ENGINE_LOG_TRACE("Creating VAO...");
    glVertexArrayVertexBuffer(
        m_vertexArrayID,
        vaoBindingPoint,
        buffer->getBufferId(),
        0,
        sizeof(Vertex));

    m_numberOfVerticesInBindedObject = static_cast<GLsizei>(buffer->getSize() / sizeof(Vertex));
    ENGINE_LOG_TRACE("VAO Created");
    glEnableVertexArrayAttrib(m_vertexArrayID, attribPos);
    glEnableVertexArrayAttrib(m_vertexArrayID, attribCol);
    glEnableVertexArrayAttrib(m_vertexArrayID, attribTexCoord);

    glVertexArrayAttribFormat(m_vertexArrayID, attribPos, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(m_vertexArrayID, attribCol, 3, GL_FLOAT, GL_FALSE, sizeof(math::Vec3));
    glVertexArrayAttribFormat(m_vertexArrayID, attribTexCoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(math::Vec3));

    glVertexArrayAttribBinding(m_vertexArrayID, attribPos, vaoBindingPoint);
    glVertexArrayAttribBinding(m_vertexArrayID, attribCol, vaoBindingPoint);
    glVertexArrayAttribBinding(m_vertexArrayID, attribTexCoord, vaoBindingPoint);
}

void OpenGLPipeline::bindInstanceBuffer(std::shared_ptr<Buffer> instanceBuffer) {
    ENGINE_LOG_TRACE("Instance buffer binding...");
    OpenGLBuffer* buffer = static_cast<OpenGLBuffer*>(instanceBuffer.get());

    if (buffer->getType() != BufferType::VERTEX) {
        ENGINE_LOG_ERROR("Buffer is not of type VERTEX");
        return;
    }

    GLuint vaoBindingPoint = 1;
    glVertexArrayVertexBuffer(
        m_vertexArrayID,
        vaoBindingPoint,
        buffer->getBufferId(),
        0,
        sizeof(InstanceData));

    glVertexArrayBindingDivisor(m_vertexArrayID, vaoBindingPoint, 1);

    for (GLuint i = 0; i < 4; ++i) {
        GLuint attribLocation = 3 + i;
        glEnableVertexArrayAttrib(m_vertexArrayID, attribLocation);
        glVertexArrayAttribFormat(m_vertexArrayID, attribLocation, 4, GL_FLOAT, GL_FALSE, i * 4 * sizeof(float));
        glVertexArrayAttribBinding(m_vertexArrayID, attribLocation, vaoBindingPoint);
    }
}

void OpenGLPipeline::bindIndexBuffer(std::shared_ptr<Buffer> indexBuffer) {
    ENGINE_LOG_TRACE("Index buffer binding...");
    OpenGLBuffer* buffer = static_cast<OpenGLBuffer*>(indexBuffer.get());

    if (buffer->getType() != BufferType::INDEX) {
        ENGINE_LOG_ERROR("Buffer is not of type INDEX");
        return;
    }

    glVertexArrayElementBuffer(m_vertexArrayID, buffer->getBufferId());
    m_numberOfIndicesInBindedObject = static_cast<GLsizei>(buffer->getSize() / sizeof(uint32_t));
}

void OpenGLPipeline::bindTexture(std::shared_ptr<Texture> texture, uint32_t slot) {
    ENGINE_LOG_TRACE("Texture binding...");
    OpenGLTexture* glTex = static_cast<OpenGLTexture*>(texture.get());
    glBindTextureUnit(slot, glTex->getTextureID());
}

} // namespace midgard::bifrost::opengl


