#pragma once
#include <cstdint>
#include <memory>

#include <glad/glad.h>

#include "bifrost/pipeline.h"

namespace midgard::bifrost::opengl {

class OpenGLPipeline : public Pipeline {
public:
    OpenGLPipeline(const PipelineInfo& info);
    ~OpenGLPipeline() override;
    
    void bindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer) override;
    void bindInstanceBuffer(std::shared_ptr<Buffer> instanceBuffer) override;
    void bindIndexBuffer(std::shared_ptr<Buffer> indexBuffer) override;
    void bindTexture(std::shared_ptr<Texture> texture, uint32_t slot = 0) override;

    GLuint getVertexArrayID() const { return m_vertexArrayID; }
    GLuint getShaderProgramID() const { return m_shaderProgamID; }

    GLsizei getBindedNumberOfVertices() const { return m_numberOfVerticesInBindedObject; }
    GLsizei getBindedNumberOfIndices() const { return m_numberOfIndicesInBindedObject; }

private:
    GLuint m_vertexArrayID = 0;
    GLuint m_shaderProgamID = 0;

    GLsizei m_numberOfVerticesInBindedObject = 0;
    GLsizei m_numberOfIndicesInBindedObject = 0;
};

} // namespace midgard::bifrost::opengl
