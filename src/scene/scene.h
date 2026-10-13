#pragma once
#include <vector>
#include <memory>

#include "scene/camera.h"

namespace midgard::renderer {
    class RenderScene;
}

namespace midgard::scene {

class SceneNode;

class SceneGraph {
public:

    void setRootNode(std::unique_ptr<SceneNode> node) { m_rootNode = std::move(node); }

    const renderer::RenderScene& generateRenderScene();

private:

    std::unique_ptr<SceneNode> m_rootNode;
    Camera m_camera;
};


} // namespace midgard::scene