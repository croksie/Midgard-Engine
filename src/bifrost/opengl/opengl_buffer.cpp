#include "bifrost/opengl/opengl_buffer.h"

namespace midgard::bifrost::opengl {

OpenGLBuffer::OpenGLBuffer(const BufferDesc& desc) : m_desc(desc) {
    glCreateBuffers(1, &m_bufferID);
    glNamedBufferStorage(m_bufferID, desc.size, desc.initData, GL_DYNAMIC_STORAGE_BIT);
}

OpenGLBuffer::~OpenGLBuffer() {
    if (m_bufferID) {
        glDeleteBuffers(1, &m_bufferID);
    }
}

void OpenGLBuffer::setData(size_t size, const void* data, size_t offset) {
    if (offset + size > m_desc.size)
        return;
    glNamedBufferSubData(m_bufferID, offset, size, data);
}

} // namespace midgard::bifrost::opengl
