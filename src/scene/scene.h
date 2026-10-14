#pragma once
#include <unordered_map>
#include <vector>
#include <memory>

#include "scene/camera.h"

namespace midgard::math {
    class Transform;
}

namespace midgard::renderer {
    class RenderScene;
}

namespace midgard::scene {

class SceneNode;
class Model;

class SceneGraph {
public:

    void setRootNode(std::unique_ptr<SceneNode> node) { m_rootNode = std::move(node); }

    const renderer::RenderScene& generateRenderScene();

private:
    std::unique_ptr<SceneNode> m_rootNode;
    Camera m_camera;

    std::unordered_map<Model*, std::vector<math::Mat4>> m_groups;
};


} // namespace midgard::scene