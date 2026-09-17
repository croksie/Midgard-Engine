#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "bifrost/opengl/opengl_buffer.h"
#include "bifrost/bifrost.h"

namespace midgard::platform {
    class Window;
}

namespace midgard::core {
    struct EngineConfig;
}

namespace midgard::bifrost::opengl {

class OpenGLBifrost : public Bifrost {
public:
    OpenGLBifrost() = default;
    ~OpenGLBifrost() override = default;

    void initialize(platform::Window* window, std::shared_ptr<core::EngineConfig> config) override;
    void shutdown() override {}

    void beginFrame() override;
    void endFrame() override;

    void clear() override;
    void draw(std::shared_ptr<Pipeline> pipeline, uint32_t count = 1) override;

    std::shared_ptr<Shader> createShader(ShaderType type, std::string source, bool isInstanced) override;
    std::shared_ptr<Pipeline> createPipeline(PipelineInfo& info) override;
    std::shared_ptr<Buffer> createBuffer(BufferDesc& desc) override;
    std::shared_ptr<Texture> createTexture(const TextureDesc& desc) override;

    void bindVertexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) override;
    void bindInstanceBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) override;
    void bindIndexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) override;
    void bindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Texture> texture, uint32_t slot = 0) override;
    void bindPipeline(Pipeline* pipeline) override;

    void setGlobalUniform(const void* data, size_t size) override;
    void setLocalUniform(const void* data, size_t size) override;

private:
    platform::Window* m_window = nullptr;
    std::shared_ptr<core::EngineConfig> m_config;

    std::unique_ptr<OpenGLBuffer> m_uniformBuffer;

    void onWindowResize(int width, int height);
};

} // namespace midgard::bifrost::opengl
