#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "math/mat4.h"

#include "bifrost/shader.h"

namespace midgard::platform {
    class Window;
}

namespace midgard::core {
    struct EngineConfig;
}

namespace midgard::bifrost {
    struct PipelineInfo;
    class Pipeline;
    struct BufferDesc;
    class Buffer;
    struct TextureDesc;
    class Texture;


/**
 * @brief Vertex structure representing a single vertex in 3D space.
 * Contains position, normals, and texture coordinates.
 */
struct Vertex {
    math::Vec3 position;
    math::Vec3 normals;
    math::Vec2 texCoords; 
};

/**
 * @brief Instance data structure for instanced rendering.
 */
struct InstanceData {
    math::Mat4 modelMatrix;
};

/**
 * @brief Main interface for the graphics API abstraction layer.
 */
class Bifrost {
public:
    virtual ~Bifrost() = default;

    /**
     * @brief Begins a new frame for rendering.
     * @note This function should be called at the start of each frame.
     */
    virtual void beginFrame() = 0;
    /**
     * @brief Ends the current frame.
     * @note This function should be called at the end of each frame.
     */
    virtual void endFrame() = 0;
    /**
     * @brief Clears the screen.
     */
    virtual void clear() = 0;

    /**
     * @brief Draws geometry using the specified pipeline.
     * @param pipeline The rendering pipeline to use.
     * @param count The number of instances to draw.
     */
    virtual void draw(std::shared_ptr<Pipeline> pipeline, uint32_t count = 1) = 0;

    /**
     * @brief Creates a new shader.
     * @param type The type of the shader.
     * @param source The source code of the shader.
     * @return A shared pointer to the created shader.
     */
    virtual std::shared_ptr<Shader> createShader(ShaderType type, std::string source, bool isInstanced) = 0;
    /**
     * @brief Creates a new rendering pipeline.
     * @param info The information for creating the pipeline.
     * @return A shared pointer to the created pipeline.
     */
    virtual std::shared_ptr<Pipeline> createPipeline(PipelineInfo& info) = 0;
    /**
     * @brief Creates a new buffer.
     * @param desc The description for creating the buffer.
     * @return A shared pointer to the created buffer.
     */
    virtual std::shared_ptr<Buffer> createBuffer(BufferDesc& desc) = 0;
    /**
     * @brief Creates a new texture.
     * @param desc The description for creating the texture.
     * @return A shared pointer to the created texture.
     */
    virtual std::shared_ptr<Texture> createTexture(const TextureDesc& desc) = 0;

    /**
     * @brief Binds a rendering pipeline for subsequent draw calls.
     * @param pipeline The pipeline to bind.
     */
    virtual void bindPipeline(Pipeline* pipeline) = 0;
    /**
     * @brief Binds a vertex buffer to the specified pipeline.
     * @param pipeline The pipeline to bind the buffer to.
     * @param buffer The buffer to bind.
     */
    virtual void bindVertexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) = 0;
    /**
     * @brief Binds an instance buffer to the specified pipeline.
     * @param pipeline The pipeline to bind the buffer to.
     * @param buffer The buffer to bind.
     */
    virtual void bindInstanceBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) = 0;
    /**
     * @brief Binds an index buffer to the specified pipeline.
     * @param pipeline The pipeline to bind the buffer to.
     * @param buffer The buffer to bind.
     */
    virtual void bindIndexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) = 0;
    /**
     * @brief Binds a texture to the specified pipeline.
     * @param pipeline The pipeline to bind the texture to.
     * @param texture The texture to bind.
     * @param slot The texture slot to bind the texture to.
     */
    virtual void bindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Texture> texture, uint32_t slot = 0) = 0;

    /**
     * @brief Set global uniform data for the shaders.
     * 
     * @param data A pointer to a Uniform Buffer.
     * @param size The size of the data in bytes.
     */
    virtual void setGlobalUniform(const void* data, size_t size) = 0;
    /**
     * @brief Set local uniform data for the shaders.
     * 
     * @param data A pointer to a Uniform Buffer.
     * @param size The size of the data in bytes.
     */
    virtual void setLocalUniform(const void* data, size_t size) = 0;

    /**
     * @brief Initialize the Bifrost API.
     * 
     * @param window The window to initialize with.
     * @param config The engine configuration.
     */
    virtual void initialize(platform::Window* window, std::shared_ptr<core::EngineConfig> config) = 0;
    /**
     * @brief Shutdown the Bifrost API.
     * @warning This function should be called after all buffers, pipelines, and textures have been destroyed.
     */
    virtual void shutdown() = 0;
};

} // namespace midgard::bifrost