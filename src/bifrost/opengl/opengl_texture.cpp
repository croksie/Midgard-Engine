#include "bifrost/opengl/opengl_texture.h"

#include "utils/log.h"

namespace midgard::bifrost::opengl {

OpenGLTexture::OpenGLTexture(const TextureDesc& desc) {
    ENGINE_LOG_TRACE("Creating texture...");
    m_desc = desc;

    GLenum internalFormat = GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;

    switch (m_desc.format) {
        case TextureFormat::RGBA8_SRGB:
            internalFormat = GL_SRGB8_ALPHA8;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case TextureFormat::RGBA8_UNORM:
            internalFormat = GL_RGBA8;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case TextureFormat::Depth24_Stencil8:
            internalFormat = GL_DEPTH24_STENCIL8;
            format = GL_DEPTH_STENCIL;
            type = GL_UNSIGNED_INT_24_8;
            break;
        case TextureFormat::Depth32F:
            internalFormat = GL_DEPTH_COMPONENT32F;
            format = GL_DEPTH_COMPONENT;
            type = GL_FLOAT;
            break;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
    glTextureStorage2D(m_textureID, 1, internalFormat, m_desc.width, m_desc.height);
    if (m_desc.initialData != nullptr) {
        glTextureSubImage2D(m_textureID, 0, 0, 0, m_desc.width, m_desc.height, format, type, m_desc.initialData);
    }

    glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    ENGINE_LOG_TRACE("Texture created");
}

OpenGLTexture::~OpenGLTexture() {
    if (m_textureID) {
        glDeleteTextures(1, &m_textureID);
    }
}

} // namespace midgard::bifrost::opengl
