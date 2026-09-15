#include "bifrost/vulkan/vulkan_bifrost.h"

#include <stdexcept>

#include "utils/log.h"
#include "core/config.h"
#include "platform/window/window.h"

#include "bifrost/vulkan/vulkan_pipeline.h"
#include "bifrost/vulkan/vulkan_buffer.h"
#include "bifrost/vulkan/vulkan_texture.h"
#include "bifrost/vulkan/vulkan_shader.h"

namespace midgard::bifrost::vulkan {

void VulkanBifrost::initialize(platform::Window *window, std::shared_ptr<core::EngineConfig> config) {
    m_window = window;
    m_config = config;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, m_config->resizable);
    // TODO : implement fullscreen mode
    
    m_ctx = createContext(window, config);

    m_window->setWindowSizeCallback([this](int width, int height) {
        this->onWindowResize(width, height);
    });

    int width, height;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_window->getNativeHandle()), &width, &height);
    m_width = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);

    createSwapchain(m_ctx, m_width, m_height, m_config->vsync);
    createDepthBuffer(m_ctx, m_width, m_height);
    createFenceAndSemaphore(m_ctx);
    createCommandPool(m_ctx);
    createCommandBuffer(m_ctx);
    createUniformBuffers(m_ctx);
    createDescriptorSetLayout(m_ctx);
    createDescriptorPool(m_ctx);
    createDescriptorSets(m_ctx);
    createPipelineLayout(m_ctx);

    ENGINE_LOG_INFO("Initialization success");
}

void VulkanBifrost::onWindowResize(int width, int height) {
    if (width == 0 || height == 0) return;
    m_config->windowWidth = static_cast<uint32_t>(width);
    m_config->windowHeight = static_cast<uint32_t>(height);

    m_framebufferResized = true;
    m_width = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);
}

void VulkanBifrost::shutdown() {
    vkDeviceWaitIdle(m_ctx.device);
    m_ctx.uniformBuffers.clear();
    for (auto& sync : m_ctx.syncObjects) {
        vkDestroySemaphore(m_ctx.device, sync.imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_ctx.device, sync.renderFinishedSemaphore, nullptr);
        vkDestroyFence(m_ctx.device, sync.inFlightFence, nullptr);
    }
    vkDestroyPipelineLayout(m_ctx.device, m_ctx.pipelineLayout, nullptr);
    vkDestroyDescriptorPool(m_ctx.device, m_ctx.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_ctx.device, m_ctx.globalDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_ctx.device, m_ctx.textureDescriptorSetLayout, nullptr);
    vkDestroyCommandPool(m_ctx.device, m_ctx.cmdPool, nullptr);
    for (auto view : m_ctx.swapchainImageViews) {
        vkDestroyImageView(m_ctx.device, view, nullptr);
    }
    vkDestroySwapchainKHR(m_ctx.device, m_ctx.swapchain, nullptr);
    for (size_t i = 0; i < m_ctx.depthImageViews.size(); i++) {
        if (m_ctx.depthImageViews[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(m_ctx.device, m_ctx.depthImageViews[i], nullptr);
        }
        if (m_ctx.depthImages[i] != VK_NULL_HANDLE) {
            vkDestroyImage(m_ctx.device, m_ctx.depthImages[i], nullptr);
        }
        if (m_ctx.depthImageMemories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(m_ctx.device, m_ctx.depthImageMemories[i], nullptr);
        }
    }
    m_ctx.depthImageViews.clear();
    m_ctx.depthImages.clear();
    m_ctx.depthImageMemories.clear();
    vkDestroyDevice(m_ctx.device, nullptr);
    vkDestroySurfaceKHR(m_ctx.instance, m_ctx.surface, nullptr);
    vkb::destroy_debug_utils_messenger(m_ctx.instance, m_ctx.debugMessenger);
    vkDestroyInstance(m_ctx.instance, nullptr);
}

void VulkanBifrost::beginFrame() {
    ENGINE_LOG_TRACE("Begin of frame");

    if (m_framebufferResized) {
        ENGINE_LOG_DEBUG("Window resized");

        cleanupSwapchain(m_ctx);
        cleanupDepthBuffer(m_ctx);

        createSwapchain(m_ctx, m_width, m_height, m_config->vsync);
        createDepthBuffer(m_ctx, m_width, m_height);
        m_framebufferResized = false;
    }

    auto& sync = m_ctx.syncObjects[m_ctx.currentFrame];
    // Wait for GPU
    vkWaitForFences(m_ctx.device, 1, &sync.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_ctx.device, 1, &sync.inFlightFence);

    // Get image from swapchain
    vkAcquireNextImageKHR(m_ctx.device, m_ctx.swapchain, UINT64_MAX, sync.imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(m_ctx.cmdBuffers[m_ctx.currentFrame], &beginInfo) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to begin recording command buffer!");
    }

    transitionImageLayout(
        m_ctx.cmdBuffers[m_ctx.currentFrame],
        m_ctx.swapchainImages[m_imageIndex],
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    transitionImageLayout(
        m_ctx.cmdBuffers[m_ctx.currentFrame],
        m_ctx.depthImages[m_ctx.currentFrame],
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        0,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_ctx.swapchainImageViews[m_imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = m_ctx.depthImageViews[m_ctx.currentFrame];
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { {0, 0}, {m_width, m_height} };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(m_ctx.cmdBuffers[m_ctx.currentFrame], &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(m_height);
    viewport.width  = static_cast<float>(m_width);
    viewport.height = -static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};

    vkCmdSetViewport(m_ctx.cmdBuffers[m_ctx.currentFrame], 0, 1, &viewport);
    vkCmdSetScissor(m_ctx.cmdBuffers[m_ctx.currentFrame], 0, 1, &scissor);
}

void VulkanBifrost::endFrame() {
    vkCmdEndRendering(m_ctx.cmdBuffers[m_ctx.currentFrame]);

    transitionImageLayout(
        m_ctx.cmdBuffers[m_ctx.currentFrame],
        m_ctx.swapchainImages[m_imageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        0
    );
    vkEndCommandBuffer(m_ctx.cmdBuffers[m_ctx.currentFrame]);

    auto& sync = m_ctx.syncObjects[m_ctx.currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Wait for image
    VkSemaphore waitSemaphores[] = { sync.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_ctx.cmdBuffers[m_ctx.currentFrame];

    VkSemaphore signalSemaphores[] = { sync.renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(m_ctx.graphicsQueue, 1, &submitInfo, sync.inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_ctx.swapchain;
    presentInfo.pImageIndices = &m_imageIndex;

    vkQueuePresentKHR(m_ctx.presentQueue, &presentInfo);

    m_ctx.currentFrame = (m_ctx.currentFrame + 1) % m_ctx.maxFramesInFlight;
    glfwPollEvents();
    ENGINE_LOG_TRACE("End of frame");
}

void VulkanBifrost::clear() {

}

void VulkanBifrost::draw(std::shared_ptr<Pipeline> pipeline, uint32_t count) {
    VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline.get());
    vkCmdDrawIndexed(m_ctx.cmdBuffers[m_ctx.currentFrame], vulkanPipeline->getBindedNumberOfIndices(), count, 0, 0, 0);
}

std::shared_ptr<Buffer> VulkanBifrost::createBuffer(BufferDesc& desc) {
    ENGINE_LOG_TRACE("Creating buffer...");
    return std::make_shared<VulkanBuffer>(desc, m_ctx);
}

void VulkanBifrost::bindVertexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) {
    ENGINE_LOG_TRACE("Binding vertex buffer...");
    VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline.get());
    vulkanPipeline->bindVertexBuffer(buffer);
}

void VulkanBifrost::bindInstanceBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) {
    ENGINE_LOG_TRACE("Binding instance buffer...");
    VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline.get());
    vulkanPipeline->bindInstanceBuffer(buffer);
}

void VulkanBifrost::bindIndexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) {
    ENGINE_LOG_TRACE("Binding index buffer...");
    VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline.get());
    vulkanPipeline->bindIndexBuffer(buffer);
}

std::shared_ptr<Texture> VulkanBifrost::createTexture(const TextureDesc& desc) {
    ENGINE_LOG_TRACE("Creating texture...");
    return std::make_shared<VulkanTexture>(desc, m_ctx);
}

void VulkanBifrost::bindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Texture> texture, uint32_t slot) {
    ENGINE_LOG_TRACE("Binding texture...");
    if (!pipeline || !texture) return;
    VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline.get());
    vulkanPipeline->bindTexture(texture, slot);
}

std::shared_ptr<Pipeline> VulkanBifrost::createPipeline(PipelineInfo &info) {
    ENGINE_LOG_TRACE("Creating pipeline...");
    return std::make_shared<VulkanPipeline>(info, m_ctx, m_ctx.pipelineLayout);
}

void VulkanBifrost::bindPipeline(Pipeline *pipeline) {
    ENGINE_LOG_TRACE("Binding pipeline...");
    VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline);
    m_currentPipeline = vulkanPipeline;
    vkCmdBindPipeline(m_ctx.cmdBuffers[m_ctx.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getPipeline());
}

std::shared_ptr<Shader> VulkanBifrost::createShader(ShaderType type, std::string source) {
    return std::make_shared<VulkanShader>(m_ctx.device, type, source);
}

void VulkanBifrost::setGlobalUniform(const void* data, size_t size) {
    m_ctx.uniformBuffers[m_ctx.currentFrame]->setData(size, data, 0);
    vkCmdBindDescriptorSets(m_ctx.cmdBuffers[m_ctx.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_ctx.pipelineLayout, 0, 1, &m_ctx.descriptorSets[m_ctx.currentFrame], 0, nullptr);
}

void VulkanBifrost::setLocalUniform(const void* data, size_t size) {
    vkCmdPushConstants(m_ctx.cmdBuffers[m_ctx.currentFrame], m_ctx.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(size), data);
}

} // namespace midgard::bifrost::vulkan