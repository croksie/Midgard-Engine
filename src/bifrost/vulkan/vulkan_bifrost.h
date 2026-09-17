#pragma once
#include <vector>
#include <array>
#include <memory>
#include <string>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "bifrost/bifrost.h"
#include "bifrost/vulkan/vulkan_initializer.h"

namespace midgard::core {
    struct EngineConfig;
}

namespace midgard::platform {
    class Window;
}

namespace midgard::bifrost::vulkan {
    class VulkanPipeline;
    class VulkanBuffer;

class VulkanBifrost : public Bifrost {
public:
    VulkanBifrost() = default;
    ~VulkanBifrost() override = default;

    void initialize(platform::Window* window, std::shared_ptr<core::EngineConfig> config) override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;

    void clear() override;
    void draw(std::shared_ptr<Pipeline> pipeline, uint32_t count = 1) override;

    std::shared_ptr<Shader> createShader(ShaderType type, std::string source, bool isInstanced) override;
    std::shared_ptr<Buffer> createBuffer(BufferDesc& desc) override;
    std::shared_ptr<Pipeline> createPipeline(PipelineInfo& info) override;
    std::shared_ptr<Texture> createTexture(const TextureDesc& desc) override;

    void bindPipeline(Pipeline* pipeline) override;
    void bindVertexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) override;
    void bindInstanceBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) override;
    void bindIndexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) override;
    void bindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Texture> texture, uint32_t slot = 0) override;

    void setGlobalUniform(const void* data, size_t size) override;
    void setLocalUniform(const void* data, size_t size) override;

private:
    platform::Window* m_window = nullptr;
    std::shared_ptr<core::EngineConfig> m_config;

    VulkanContext m_ctx{};
    uint32_t m_imageIndex = 0;

    VulkanPipeline* m_currentPipeline = nullptr;

    void onWindowResize(int width, int height);
    bool m_framebufferResized = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace midgard::bifrost::vulkan