#include "scene/scene.h"
#include "scene/scene_node.h"
#include <rendering/render_scene.h>

namespace midgard::scene {

    const renderer::RenderScene &SceneGraph::generateRenderScene()
    {
        renderer::RenderScene rs = renderer::RenderScene();
        rs.setCamera(m_camera);

        for (auto& [key, value] : m_groups) {
            value.clear();
        }
        m_rootNode->collectTransforms(math::Mat4::identity(), m_groups);

    }

} // namespace midgard::scene