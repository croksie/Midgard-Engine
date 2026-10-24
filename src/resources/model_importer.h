#pragma once
#include <memory>
#include <string>

namespace midgard::bifrost {
class Bifrost;
class PipelineManager;
} // namespace midgard::bifrost

namespace midgard::scene {
class SceneNode;
}

namespace midgard::resource {

class ModelImporter {
public:
    static std::shared_ptr<scene::SceneNode> loadModel(const std::string& filePath, bifrost::Bifrost* bifrost,
                                                       bifrost::PipelineManager& pipelineManager,
                                                       const std::string& shaderPath = "shaders/base.hlsl");
};

} // namespace midgard::resource
