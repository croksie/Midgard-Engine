#pragma once
#include <vector>
#include <memory>

#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "core/config.h"

#include "bifrost/vulkan/vulkan_buffer.h"

namespace midgard::platform {
    class Window;
}

namespace midgard::bifrost::vulkan {

struct FrameSyncObjects {
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE; // When swapChain gives image
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE; // When GPU finishes render
    VkFence inFlightFence = VK_NULL_HANDLE;
};

struct VulkanContext {
    VulkanContext(uint32_t maxFramesInFlight = 3) 
        : maxFramesInFlight(maxFramesInFlight),
          syncObjects(maxFramesInFlight),
          cmdBuffers(maxFramesInFlight),
          descriptorSets(maxFramesInFlight) {}

    uint32_t maxFramesInFlight = 3;
    uint32_t currentFrame = 0;

    uint32_t maxTextureCount = 256;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;

    // Swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    // DepthBuffer
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemories;
    std::vector<VkImageView> depthImageViews;

    // Fences / Semaphores
    std::vector<FrameSyncObjects> syncObjects;

    // CommandPool / CommandBuffer
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmdBuffers;
    
    // Uniform Buffers
    std::vector<std::unique_ptr<VulkanBuffer>> uniformBuffers;

    // Global Descriptors
    VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    // Texture Descriptors
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;

    // Pipeline Layout
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

// Initialize
VulkanContext createContext(platform::Window* window, std::shared_ptr<core::EngineConfig> config);
void createSwapchain(VulkanContext& ctx, uint32_t width, uint32_t height, bool vsync);
void createDepthBuffer(VulkanContext& ctx, uint32_t width, uint32_t height);
void createFenceAndSemaphore(VulkanContext& ctx);
void createCommandPool(VulkanContext& ctx);
void createCommandBuffer(VulkanContext& ctx);
void createUniformBuffers(VulkanContext& ctx);
void createDescriptorSetLayout(VulkanContext& ctx);
void createDescriptorPool(VulkanContext& ctx);
void createDescriptorSets(VulkanContext& ctx);
void createPipelineLayout(VulkanContext& ctx);

// CleanUp
void cleanupSwapchain(VulkanContext& ctx);
void cleanupDepthBuffer(VulkanContext& ctx);

// Utility
uint32_t findMemoryType(const VulkanContext& ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkCommandBuffer beginSingleTimeCommands(const VulkanContext& ctx);
void endSingleTimeCommands(const VulkanContext& ctx, VkCommandBuffer commandBuffer);
void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                           VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess,
                           VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

} // namespace midgard::bifrost::vulkan