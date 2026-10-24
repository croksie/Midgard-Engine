#pragma once
#include <memory>
#include <vector>

#include "bifrost/bifrost.h"

#include "scene/camera.h"
#include "scene/model.h"

namespace midgard::core {
struct EngineConfig;
}

namespace midgard::platform {
class Window;
}

namespace midgard::bifrost {
class Buffer;
class Texture;
} // namespace midgard::bifrost

#include "bifrost/pipeline_manager.h"

namespace midgard::scene {
class SceneGraph;
}

namespace midgard::render {

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    void initialize(platform::Window* window, std::shared_ptr<core::EngineConfig> config);
    void render();
    void shutdown();

    bifrost::PipelineManager& getPipelineManager() { return m_pipelineManager; }

private:
    std::shared_ptr<core::EngineConfig> m_config;
    std::unique_ptr<bifrost::Bifrost> m_rhi;

    std::shared_ptr<scene::SceneGraph> m_sceneGraph;
    scene::Camera m_camera;

    bifrost::PipelineManager m_pipelineManager;
    std::vector<std::shared_ptr<bifrost::Buffer>> m_instanceBuffers;
    uint32_t m_currentFrame = 0;
    static constexpr size_t MAX_INSTANCES = 50000;

    uint32_t m_lastWidth = 0;
    uint32_t m_lastHeight = 0;

    float m_lastFrameTime = 0.0f;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouse = true;
};

} // namespace midgard::render