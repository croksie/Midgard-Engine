#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include "utils/log.h"

#include "bifrost/shader.h"

namespace midgard::bifrost::vulkan {

inline VkShaderStageFlagBits shaderTypeToVKType(ShaderType type) {
    switch (type) {
        case ShaderType::VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
        default: 
            ENGINE_LOG_ERROR("Invalid shader type");
            return VK_SHADER_STAGE_ALL;
    }
}

class VulkanShader : public Shader {
public:
    VulkanShader(VkDevice device, ShaderType type, const std::vector<uint32_t>& spirvCode, const std::string& entryPoint = "main");
    VulkanShader(VkDevice device, ShaderType type, const std::string& source, const std::string& entryPoint = "main");
    ~VulkanShader() override;

    VkPipelineShaderStageCreateInfo getStageCreateInfo() const;
    VkShaderModule getModule() const { return m_module; }

private:
    std::vector<uint32_t> compileGLSLToSPIRV(
        const std::string& source,
        ShaderType kind,
        const std::string& entryPoint = "main");

    VkDevice m_device = VK_NULL_HANDLE;
    ShaderType m_type;
    VkShaderModule m_module = VK_NULL_HANDLE;
    std::string m_entryPoint;
};

} // namespace midgard::bifrost::vulkan