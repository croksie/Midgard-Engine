#pragma once
#include <memory>

#include "math/vec3.h"
#include "math/vec4.h"

namespace midgard::bifrost {
class Pipeline;
class BindGroup;
} // namespace midgard::bifrost

namespace midgard::resource {

enum class MaterialTextureSlot : uint8_t { BASE_COLOR = 0, NORMAL = 1, METALLIC_ROUGHNESS = 2, OCCLUSION = 3, EMISSIVE = 4, COUNT = 5 };

struct alignas(16) MaterialData { // Align as 16 bits for HLSL
    math::Vec4 baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
    math::Vec3 emissiveFactor{0.0F, 0.0F, 0.0F};
    float roughnessFactor{1.0F};
    float metallicFactor{0.0F};
    float alphaCutoff{0.5F};
    float occlusionStrength{1.0F};
    float _padding{0.0F};
};

class Material {
public:
    Material(const std::shared_ptr<bifrost::Pipeline>& singlePipeline,
             const std::shared_ptr<bifrost::Pipeline>& instancedPipeline,
             const std::shared_ptr<bifrost::BindGroup>& bindGroup)
        : m_singlePipeline(singlePipeline),
          m_instancedPipeline(instancedPipeline),
          m_bindGroup(bindGroup) {}

    [[nodiscard]] std::shared_ptr<bifrost::Pipeline> getPipeline(bool instanced = false) const {
        return instanced ? m_instancedPipeline : m_singlePipeline;
    }

    [[nodiscard]] std::shared_ptr<bifrost::BindGroup> getBindGroup() const {
        return m_bindGroup;
    }

private:
    std::shared_ptr<bifrost::Pipeline> m_singlePipeline;
    std::shared_ptr<bifrost::Pipeline> m_instancedPipeline;
    std::shared_ptr<bifrost::BindGroup> m_bindGroup;
};

} // namespace midgard::resource
