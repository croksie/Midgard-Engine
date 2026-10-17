#include "bifrost/vulkan/vulkan_buffer.h"

#include <cstring>
#include <stdexcept>

#include "utils/log.h"

#include "bifrost/vulkan/vulkan_initializer.h"

namespace midgard::bifrost::vulkan {

VulkanBuffer::VulkanBuffer(const BufferDesc& desc, const VulkanContext& ctx) : m_ctx(ctx), m_desc(desc) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = bufferTypeToVulkanType(m_desc.type);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(m_ctx.device, &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to create buffer. Vulkan error Code : {}", static_cast<int>(result));
    }

    // Get Memory Requirement and Allocate Memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ctx.device, m_buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(ctx, memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    result = vkAllocateMemory(ctx.device, &allocInfo, nullptr, &m_bufferMemory);
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to allocate buffer memory. Vulkan error Code : {}", static_cast<int>(result));
    }

    result = vkBindBufferMemory(ctx.device, m_buffer, m_bufferMemory, 0);
    if (result != VK_SUCCESS) {
        ENGINE_LOG_ERROR("Failed to bind buffer memory. Vulkan error Code : {}", static_cast<int>(result));
    }

    result = vkMapMemory(m_ctx.device, m_bufferMemory, 0, desc.size, 0, &m_bufferData);
    if (result != VK_SUCCESS) {
        ENGINE_LOG_CRITICAL("Failed to map memory. Vulkan error Code : {}", static_cast<int>(result));
    }

    // Copy data to buffer if data is provided
    if (desc.initData != nullptr) {
        memcpy(m_bufferData, desc.initData, (size_t)bufferInfo.size);
    }
}

VulkanBuffer::~VulkanBuffer() {
    if (m_buffer != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_ctx.device);
        vkUnmapMemory(m_ctx.device, m_bufferMemory);
        vkDestroyBuffer(m_ctx.device, m_buffer, nullptr);
        vkFreeMemory(m_ctx.device, m_bufferMemory, nullptr);
    }
}

void VulkanBuffer::setData(size_t size, const void* data, size_t offset) {
    if (offset + size > m_desc.size) {
        ENGINE_LOG_ERROR("Buffer overflow, data was not set.");
        return;
    }
    memcpy(m_bufferData, data, (size_t)size);
}

} // namespace midgard::bifrost::vulkan
