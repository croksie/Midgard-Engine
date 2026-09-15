#include "bifrost/vulkan/vulkan_initializer.h"

#include <array>
#include <stdexcept>

#include "math/math.h"
#include "utils/log.h"

#include "platform/window/window.h"



namespace midgard::bifrost::vulkan {

VulkanContext createContext(platform::Window *window, std::shared_ptr<core::EngineConfig> config) {
    window->initializeWindow(config->windowWidth, config->windowHeight, config->windowTitle.c_str());

    VulkanContext ctx(config->maxFramesInFlight);

    ENGINE_LOG_TRACE("Creating instance...");
    // Use vkbootstrap to initialize Vulkan
    vkb::InstanceBuilder instBuilder;
    auto instResult = instBuilder
        .set_app_name("Midgard")
        #ifdef DEBUG
            .request_validation_layers(true) 
            .use_default_debug_messenger()
        #endif
        .require_api_version(1, 3, 0) // Force Vulkan 1.3 in order to use Dynamic Rendering
        .build();

    if (!instResult) {
        ENGINE_LOG_CRITICAL("Failed to create instance : {}",instResult.error().message());
    }
    vkb::Instance vkbInst = instResult.value();
    ctx.instance = vkbInst.instance;
    ctx.debugMessenger = vkbInst.debug_messenger;

    // Create surface
    glfwCreateWindowSurface(ctx.instance, static_cast<GLFWwindow*>(window->getNativeHandle()), nullptr, &ctx.surface);

    // Use Vulkan 1.3 features (dynamicRendering, etc)
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;    // Suppress RenderPass / Framebuffers
    features13.synchronization2 = VK_TRUE;    // Simplified Sync

    ENGINE_LOG_TRACE("Choosing physical device...");
    // Select Physical Device
    vkb::PhysicalDeviceSelector selector{ vkbInst };
    auto physResult = selector
        .set_surface(ctx.surface)
        .set_minimum_version(1, 3)
        .set_required_features_13(features13)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .select();

    if (!physResult) {
        ENGINE_LOG_CRITICAL("Failed to choose physical device : {}",physResult.error().message());
    }
    vkb::PhysicalDevice vkbPhysDevice = physResult.value();
    ctx.physicalDevice = vkbPhysDevice.physical_device;

    ENGINE_LOG_TRACE("Creating logical device and queue...");
    // Create logical device and queue
    vkb::DeviceBuilder devBuilder{ vkbPhysDevice };
    auto devResult = devBuilder.build();
    if (!devResult) {
        ENGINE_LOG_CRITICAL("Failed to create logical device : {}", devResult.error().message());
    }
    vkb::Device vkbDevice = devResult.value();
    ctx.device = vkbDevice.device;

    ctx.graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    ctx.graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    ctx.presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();

    return ctx;
}

// Swapchain
void createSwapchain(VulkanContext &ctx, uint32_t width, uint32_t height, bool vsync) {
    ENGINE_LOG_TRACE("Creating swapchain...");
    vkb::SwapchainBuilder scBuilder{ ctx.physicalDevice, ctx.device, ctx.surface };
    auto scResult = scBuilder
        .set_desired_extent(width, height)
        .set_desired_present_mode(vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
        .build();

    if (!scResult) {
        ENGINE_LOG_CRITICAL("Failed to create swapchain : {}", scResult.error().message());
    }
    vkb::Swapchain vkbSwapchain = scResult.value();
    
    ctx.swapchain = vkbSwapchain.swapchain;
    ctx.swapchainFormat = vkbSwapchain.image_format;
    ctx.swapchainImages = vkbSwapchain.get_images().value();
    ctx.swapchainImageViews = vkbSwapchain.get_image_views().value();
}

// Depth Buffer
void createDepthBuffer(VulkanContext &ctx, uint32_t width, uint32_t height) {
    ENGINE_LOG_TRACE("Checking depth format...");
    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx.physicalDevice, format, &props);

        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            ctx.depthFormat = format;
            break;
        }
    }
    if (ctx.depthFormat == VK_FORMAT_UNDEFINED) {
        ENGINE_LOG_CRITICAL("No compatible depth format found");
    }

    ENGINE_LOG_TRACE("Creating depth resources...");
    ctx.depthImages.resize(ctx.maxFramesInFlight);
    ctx.depthImageMemories.resize(ctx.maxFramesInFlight);
    ctx.depthImageViews.resize(ctx.maxFramesInFlight);

    for (size_t i = 0; i < ctx.maxFramesInFlight; i++) {
        VkImageCreateInfo depthImageInfo{};
        depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageInfo.extent.width = static_cast<uint32_t>(width);
        depthImageInfo.extent.height = static_cast<uint32_t>(height);
        depthImageInfo.extent.depth = 1;
        depthImageInfo.mipLevels = 1;
        depthImageInfo.arrayLayers = 1;
        depthImageInfo.format = ctx.depthFormat;
        depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(ctx.device, &depthImageInfo, nullptr, &ctx.depthImages[i]) != VK_SUCCESS) {
            ENGINE_LOG_CRITICAL("Failed to create depth image");
        }

        VkMemoryRequirements depthMemReqs;
        vkGetImageMemoryRequirements(ctx.device, ctx.depthImages[i], &depthMemReqs);

        VkMemoryAllocateInfo depthAllocInfo{};
        depthAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        depthAllocInfo.allocationSize = depthMemReqs.size;
        depthAllocInfo.memoryTypeIndex = findMemoryType(ctx, depthMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(ctx.device, &depthAllocInfo, nullptr, &ctx.depthImageMemories[i]) != VK_SUCCESS) {
            ENGINE_LOG_CRITICAL("Failed to allocate depth image memory");
        }

        vkBindImageMemory(ctx.device, ctx.depthImages[i], ctx.depthImageMemories[i], 0);

        VkImageViewCreateInfo depthViewInfo{};
        depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image = ctx.depthImages[i];
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = ctx.depthFormat;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel = 0;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(ctx.device, &depthViewInfo, nullptr, &ctx.depthImageViews[i]) != VK_SUCCESS) {
            ENGINE_LOG_CRITICAL("Failed to create depth image view");
        }
    }
}

void createFenceAndSemaphore(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Creating fence and semaphore...");
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < ctx.maxFramesInFlight; i++) {
        vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.syncObjects[i].imageAvailableSemaphore);
        vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.syncObjects[i].renderFinishedSemaphore);
        vkCreateFence(ctx.device, &fenceInfo, nullptr, &ctx.syncObjects[i].inFlightFence);
    }
}

void createCommandPool(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Creating command pool...");
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ctx.graphicsQueueFamily;

    vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.cmdPool);
}

void createCommandBuffer(VulkanContext &ctx) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = ctx.maxFramesInFlight; 

    if (vkAllocateCommandBuffers(ctx.device, &allocInfo, ctx.cmdBuffers.data()) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to allocate command buffers");
    }
}

void createUniformBuffers(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Creating uniform buffer...");
    BufferDesc desc{};
    desc.size = 2048;
    desc.type = BufferType::UNIFORM;
    for (size_t i = 0; i < ctx.maxFramesInFlight; i++) {
        ctx.uniformBuffers.emplace_back(std::make_unique<VulkanBuffer>(desc, ctx));
    }
}

void createDescriptorSetLayout(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Creating descriptor set layouts...");

    // Set 0 : Global Uniform
    VkDescriptorSetLayoutBinding globalLayoutBinding{};
    globalLayoutBinding.binding = 0;
    globalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalLayoutBinding.descriptorCount = 1;
    globalLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    globalLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
    globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    globalLayoutInfo.bindingCount = 1;
    globalLayoutInfo.pBindings = &globalLayoutBinding;

    if (vkCreateDescriptorSetLayout(ctx.device, &globalLayoutInfo, nullptr, &ctx.globalDescriptorSetLayout) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create global descriptor set layout");
    }

    // Set 1 : Material / Texture
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(ctx.device, &textureLayoutInfo, nullptr, &ctx.textureDescriptorSetLayout) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create texture descriptor set layout");
    }
}

void createDescriptorPool(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Creating descriptor pool...");
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(ctx.maxFramesInFlight);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(ctx.maxTextureCount);

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = static_cast<uint32_t>(ctx.maxFramesInFlight + ctx.maxTextureCount);

    if (vkCreateDescriptorPool(ctx.device, &descriptorPoolInfo, nullptr, &ctx.descriptorPool) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create descriptor pool");
    }
}

void createDescriptorSets(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Allocating descriptor sets...");
    std::vector<VkDescriptorSetLayout> layouts(ctx.maxFramesInFlight, ctx.globalDescriptorSetLayout);

    VkDescriptorSetAllocateInfo descriptorAllocInfo{};
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.descriptorPool = ctx.descriptorPool;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(ctx.maxFramesInFlight);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(ctx.device, &descriptorAllocInfo, ctx.descriptorSets.data()) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to allocate descriptor sets");
    }

    ENGINE_LOG_TRACE("Writing descriptor sets...");
    for (size_t i = 0; i < ctx.maxFramesInFlight; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = *ctx.uniformBuffers[i]->getBufferHandle();
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = ctx.descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(ctx.device, 1, &descriptorWrite, 0, nullptr);
    }
}

void createPipelineLayout(VulkanContext &ctx) {
    ENGINE_LOG_TRACE("Creating pipeline layout...");
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(math::Mat4);

    std::array<VkDescriptorSetLayout, 2> setLayouts = {
        ctx.globalDescriptorSetLayout,
        ctx.textureDescriptorSetLayout
    };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(ctx.device, &layoutInfo, nullptr, &ctx.pipelineLayout) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create pipeline layout");
    }
}

/* ------------ Cleanup ------------ */

void cleanupSwapchain(VulkanContext &ctx) {
    vkDeviceWaitIdle(ctx.device);

    for (auto view : ctx.swapchainImageViews) {
        vkDestroyImageView(ctx.device, view, nullptr);
    }
    ctx.swapchainImageViews.clear();
    ctx.swapchainImages.clear();

    if (ctx.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
        ctx.swapchain = VK_NULL_HANDLE;
    }
}

void cleanupDepthBuffer(VulkanContext& ctx) {
    for (size_t i = 0; i < ctx.depthImageViews.size(); i++) {
        if (ctx.depthImageViews[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(ctx.device, ctx.depthImageViews[i], nullptr);
            ctx.depthImageViews[i] = VK_NULL_HANDLE;
        }
    }

    for (size_t i = 0; i < ctx.depthImages.size(); i++) {
        if (ctx.depthImages[i] != VK_NULL_HANDLE) {
            vkDestroyImage(ctx.device, ctx.depthImages[i], nullptr);
            ctx.depthImages[i] = VK_NULL_HANDLE;
        }
    }

    for (size_t i = 0; i < ctx.depthImageMemories.size(); i++) {
        if (ctx.depthImageMemories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(ctx.device, ctx.depthImageMemories[i], nullptr);
            ctx.depthImageMemories[i] = VK_NULL_HANDLE;
        }
    }

    ctx.depthImageViews.clear();
    ctx.depthImages.clear();
    ctx.depthImageMemories.clear();
}

/* ------------ Utility ------------ */

uint32_t findMemoryType(const VulkanContext& ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    ENGINE_LOG_CRITICAL("Failed to find suitable memory type");
}

VkCommandBuffer beginSingleTimeCommands(const VulkanContext& ctx) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = ctx.cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void endSingleTimeCommands(const VulkanContext& ctx, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue);

    vkFreeCommandBuffers(ctx.device, ctx.cmdPool, 1, &commandBuffer);
}

void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                           VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess,
                           VkImageAspectFlags aspectMask) {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

} // namespace midgard::bifrost::vulkan