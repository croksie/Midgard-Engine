#include "bifrost/vulkan/vulkan_bind_group.h"

#include <vector>

#include "utils/log.h"

#include "bifrost/vulkan/vulkan_buffer.h"
#include "bifrost/vulkan/vulkan_initializer.h"
#include "bifrost/vulkan/vulkan_texture.h"

namespace midgard::bifrost::vulkan {

VulkanBindGroup::VulkanBindGroup(const BindGroupDesc& desc, VulkanContext& ctx) : BindGroup(desc), m_ctx(&ctx) {
    if (m_ctx->device == VK_NULL_HANDLE || m_ctx->descriptorPool == VK_NULL_HANDLE) {
        ENGINE_LOG_ERROR("Cannot create VulkanBindGroup: Invalid Vulkan device or descriptor pool");
        return;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_ctx->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_ctx->textureDescriptorSetLayout;

    VkResult allocResult = vkAllocateDescriptorSets(m_ctx->device, &allocInfo, &m_descriptorSet);
    if (allocResult != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to allocate descriptor set for VulkanBindGroup (error: {})", static_cast<int>(allocResult));
        return;
    }

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(m_desc.entries.size());
    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(m_desc.entries.size());
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(m_desc.entries.size());

    for (const auto& entry : m_desc.entries) {
        if (entry.buffer) {
            auto vulkanBuffer = std::static_pointer_cast<VulkanBuffer>(entry.buffer);
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = *vulkanBuffer->getBufferHandle();
            bufInfo.offset = 0;
            bufInfo.range = vulkanBuffer->getSize();
            bufferInfos.push_back(bufInfo);
        } else if (entry.texture) {
            auto vulkanTexture = std::static_pointer_cast<VulkanTexture>(entry.texture);
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = vulkanTexture->getImageView();
            imgInfo.sampler = vulkanTexture->getSampler();
            imageInfos.push_back(imgInfo);
        }
    }

    size_t bufferIndex = 0;
    size_t imageIndex = 0;
    for (const auto& entry : m_desc.entries) {
        if (entry.buffer) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_descriptorSet;
            write.dstBinding = entry.binding;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferInfos[bufferIndex++];
            writes.push_back(write);
        } else if (entry.texture) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_descriptorSet;
            write.dstBinding = entry.binding;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageInfos[imageIndex++];
            writes.push_back(write);
        }
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(m_ctx->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

VulkanBindGroup::~VulkanBindGroup() {
    if (m_descriptorSet != VK_NULL_HANDLE && m_ctx != nullptr && m_ctx->device != VK_NULL_HANDLE &&
        m_ctx->descriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_ctx->device, m_ctx->descriptorPool, 1, &m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }
}

} // namespace midgard::bifrost::vulkan
