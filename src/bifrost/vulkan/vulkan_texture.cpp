#include "bifrost/vulkan/vulkan_texture.h"

#include <stdexcept>

#include "utils/log.h"

#include "bifrost/vulkan/vulkan_buffer.h"
#include "bifrost/vulkan/vulkan_initializer.h"

namespace midgard::bifrost::vulkan {

VulkanTexture::VulkanTexture(const TextureDesc& desc, VulkanContext& ctx) : m_desc(desc), m_ctx(&ctx) {
    ENGINE_LOG_INFO("Creating texture...");

    // Create Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { m_desc.width, m_desc.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = textureFormatToVulkanFormat(m_desc.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_ctx->device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create image");
    }

    // Allocate and bind device memory for Image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_ctx->device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(*m_ctx, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_ctx->device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to allocate image memory");
    }
    vkBindImageMemory(m_ctx->device, m_image, m_imageMemory, 0);

    // Staging buffer and copy
    if (desc.initialData != nullptr) {
        BufferDesc bufferDesc{};
        bufferDesc.type = BufferType::TRANSFER;
        bufferDesc.size = desc.size;
        bufferDesc.initData = desc.initialData;
        VulkanBuffer stagingBuffer(bufferDesc, *m_ctx);

        VkCommandBuffer cmd = beginSingleTimeCommands(*m_ctx);

        // Transition: UNDEFINED -> TRANSFER_DST_OPTIMAL
        transitionImageLayout(
            cmd,
            m_image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            0,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = { m_desc.width, m_desc.height, 1 };

        vkCmdCopyBufferToImage(cmd, *stagingBuffer.getBufferHandle(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        transitionImageLayout(
            cmd,
            m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        endSingleTimeCommands(*m_ctx, cmd);
    }

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = textureFormatToVulkanFormat(m_desc.format);
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_ctx->device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create image view");
    }

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(m_ctx->device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to create sampler");
    }

    // Allocate descriptor set (Set 1)
    VkDescriptorSetAllocateInfo descriptorAllocInfo{};
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.descriptorPool = m_ctx->descriptorPool;
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.pSetLayouts = &m_ctx->textureDescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_ctx->device, &descriptorAllocInfo, &m_descriptorSet) != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to allocate descriptor set for texture");
    }

    // Write descriptor set
    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = m_imageView;
    descriptorImageInfo.sampler = m_textureSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descriptorImageInfo;

    vkUpdateDescriptorSets(m_ctx->device, 1, &descriptorWrite, 0, nullptr);

    ENGINE_LOG_INFO("Texture created successfully");
}

VulkanTexture::~VulkanTexture() {
    if (m_descriptorSet != VK_NULL_HANDLE && m_ctx != nullptr && m_ctx->descriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_ctx->device, m_ctx->descriptorPool, 1, &m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }
    if (m_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_ctx->device, m_textureSampler, nullptr);
    }
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_ctx->device, m_imageView, nullptr);
    }
    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_ctx->device, m_image, nullptr);
    }
    if (m_imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_ctx->device, m_imageMemory, nullptr);
    }
}

} // namespace midgard::bifrost::vulkan