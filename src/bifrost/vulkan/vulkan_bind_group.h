#pragma once

#include <vulkan/vulkan.h>

#include "bifrost/bind_group.h"

namespace midgard::bifrost::vulkan {
struct VulkanContext;

class VulkanBindGroup : public BindGroup {
public:
    VulkanBindGroup(const BindGroupDesc& desc, VulkanContext& ctx);
    ~VulkanBindGroup() override;

    [[nodiscard]] VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }

private:
    VulkanContext* m_ctx = nullptr;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};

} // namespace midgard::bifrost::vulkan
