#pragma once
#include <vulkan/vulkan.h>

#include "utils/log.h"
#include "bifrost/texture.h"

namespace midgard::bifrost::vulkan {

struct VulkanContext;

inline VkFormat textureFormatToVulkanFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::Depth24_Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
        default:
            ENGINE_LOG_ERROR("Invalid texture format");
            return VK_FORMAT_UNDEFINED;
    }
}

class VulkanTexture : public Texture {
public:
    VulkanTexture(const TextureDesc& desc, VulkanContext& ctx);
    ~VulkanTexture() override;

    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_textureSampler; }
    VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }

private:
    VulkanContext* m_ctx = nullptr;
    TextureDesc m_desc;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};

} // namespace midgard::bifrost::vulkan