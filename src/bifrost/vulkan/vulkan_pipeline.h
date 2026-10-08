#pragma once
#include <memory>
#include <cstdint>

#include <vulkan/vulkan.h>

#include "bifrost/pipeline.h"

namespace midgard::bifrost::vulkan {
    struct VulkanContext;

class VulkanPipeline : public Pipeline {
public:
    VulkanPipeline() = delete;
    VulkanPipeline(const PipelineInfo& info, VulkanContext& ctx, VkPipelineLayout pipelineLayout);
    ~VulkanPipeline() override;

    void bindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer) override;
    void bindInstanceBuffer(std::shared_ptr<Buffer> instanceBuffer) override;
    void bindIndexBuffer(std::shared_ptr<Buffer> indexBuffer) override;
    void bindTexture(std::shared_ptr<Texture> texture, uint32_t slot = 0) override;

    VkPipeline getPipeline() const { return m_pipeline; }
    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

    uint32_t getBindedNumberOfVertices() const { return m_numberOfVerticesInBindedObject; }
    uint32_t getBindedNumberOfIndices() const { return m_numberOfIndicesInBindedObject; }

private:
    VulkanContext* m_ctx = nullptr;
    bool m_useInstance = false;

    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    uint32_t m_numberOfVerticesInBindedObject = 0;
    uint32_t m_numberOfIndicesInBindedObject = 0;
};

} // namespace midgard::bifrost::vulkan