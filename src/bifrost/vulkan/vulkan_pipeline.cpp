#include "bifrost/vulkan/vulkan_pipeline.h"

#include "utils/log.h"

#include "bifrost/bifrost.h"
#include "bifrost/vulkan/vulkan_initializer.h"
#include "bifrost/vulkan/vulkan_buffer.h"
#include "bifrost/vulkan/vulkan_shader.h"
#include "bifrost/vulkan/vulkan_texture.h"

namespace midgard::bifrost::vulkan {

VulkanPipeline::VulkanPipeline(PipelineInfo& info, VulkanContext& ctx, VkPipelineLayout pipelineLayout) 
    : m_ctx(&ctx), m_pipelineLayout(pipelineLayout) 
{
    // Shaders
    auto vertShader = std::static_pointer_cast<VulkanShader>(info.vertexShader);
    auto fragShader = std::static_pointer_cast<VulkanShader>(info.fragmentShader);
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShader->getStageCreateInfo(),
        fragShader->getStageCreateInfo()
    };

    // VertexDescription
    std::vector<VkVertexInputBindingDescription> bindingDescs;
    std::vector<VkVertexInputAttributeDescription> attribDescs;

    // Binding 0 : Vertex
    bindingDescs.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    // Vertex Attributes (Position, Color, TexCoord)
    attribDescs.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0});
    attribDescs.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)});
    attribDescs.push_back({2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float)});
    
    if (info.useInstance) {
        m_useInstance = true;
        // Binding 1 : Instance
        bindingDescs.push_back({1, static_cast<uint32_t>(sizeof(InstanceData)), VK_VERTEX_INPUT_RATE_INSTANCE});
        // Instance Attributes (Matrix)
        attribDescs.push_back({3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 0});
        attribDescs.push_back({4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 4});
        attribDescs.push_back({5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 8});
        attribDescs.push_back({6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 12});
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport and Scissor dynamic state
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dynamic Rendering Attachment Info (Vulkan 1.3)
    VkPipelineRenderingCreateInfo renderingCreateInfo{};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &ctx.swapchainFormat;
    renderingCreateInfo.depthAttachmentFormat = ctx.depthFormat;

    // Create Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    ENGINE_LOG_TRACE("Creating pipeline...");
    VkResult result = vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("vkCreateGraphicsPipelines failed with error: {}", static_cast<int>(result));
    } else {
        ENGINE_LOG_TRACE("Pipeline created");
    }
}

VulkanPipeline::~VulkanPipeline() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_ctx->device);
        vkDestroyPipeline(m_ctx->device, m_pipeline, nullptr);
    }
}

void VulkanPipeline::bindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer) {
    VulkanBuffer* vulkanVertexBuffer = static_cast<VulkanBuffer*>(vertexBuffer.get());

    vkCmdBindPipeline(m_ctx->cmdBuffers[m_ctx->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_ctx->cmdBuffers[m_ctx->currentFrame], 0, 1, vulkanVertexBuffer->getBufferHandle(), offsets);

    m_numberOfVerticesInBindedObject = static_cast<uint32_t>(vulkanVertexBuffer->getSize() / sizeof(Vertex));
}

void VulkanPipeline::bindInstanceBuffer(std::shared_ptr<Buffer> instanceBuffer) {
    if (!m_useInstance) {
        ENGINE_LOG_ERROR("Pipeline isn't created with instance support");
        return;
    }
    VulkanBuffer* vulkanInstanceBuffer = static_cast<VulkanBuffer*>(instanceBuffer.get());
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_ctx->cmdBuffers[m_ctx->currentFrame], 1, 1, vulkanInstanceBuffer->getBufferHandle(), offsets);
}

void VulkanPipeline::bindIndexBuffer(std::shared_ptr<Buffer> indexBuffer) {
    VulkanBuffer* vulkanIndexBuffer = static_cast<VulkanBuffer*>(indexBuffer.get());
    vkCmdBindIndexBuffer(m_ctx->cmdBuffers[m_ctx->currentFrame], *vulkanIndexBuffer->getBufferHandle(), 0, VK_INDEX_TYPE_UINT32);
    m_numberOfIndicesInBindedObject = static_cast<uint32_t>(vulkanIndexBuffer->getSize() / sizeof(uint32_t));
}

void VulkanPipeline::bindTexture(std::shared_ptr<Texture> texture, uint32_t slot) {
    ENGINE_LOG_TRACE("Binding texture...");
    if (!texture) return;
    VulkanTexture* vulkanTex = static_cast<VulkanTexture*>(texture.get());
    VkDescriptorSet texSet = vulkanTex->getDescriptorSet();
    if (texSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(m_ctx->cmdBuffers[m_ctx->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &texSet, 0, nullptr);
    }
}

} // namespace midgard::bifrost::vulkan