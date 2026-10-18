#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "scene/camera.h"
#include "scene/scene_node.h"

namespace midgard::renderer {
struct RenderScene;
}

namespace midgard::scene {

class Model;

class SceneGraph {
public:
    void setRootNode(std::unique_ptr<SceneNode> node) { m_rootNode = std::move(node); }

    renderer::RenderScene generateRenderScene();

private:
    std::unique_ptr<SceneNode> m_rootNode;
    Camera m_camera;

    std::unordered_map<Model*, std::vector<math::Mat4>> m_groups;
};

} // namespace midgard::scene