#pragma once
#include <memory>

#include "resources/default.h"

namespace midgard::bifrost {
class Pipeline;
}

namespace midgard::resource {

class Material {
public:
    Material(std::shared_ptr<bifrost::Pipeline> singlePipeline, std::shared_ptr<bifrost::Pipeline> instancedPipeline,
             std::shared_ptr<bifrost::Texture> texture = DefaultResources::getDefaultTexture())
        : m_singlePipeline(singlePipeline), m_instancedPipeline(instancedPipeline), m_texture(texture) {}

    std::shared_ptr<bifrost::Pipeline> getPipeline(bool instanced = false) const {
        return instanced ? m_instancedPipeline : m_singlePipeline;
    }
    std::shared_ptr<bifrost::Texture> getTexture() const { return m_texture; }

private:
    std::shared_ptr<bifrost::Pipeline> m_singlePipeline;
    std::shared_ptr<bifrost::Pipeline> m_instancedPipeline;
    std::shared_ptr<bifrost::Texture> m_texture;
};

} // namespace midgard::resource