#include "scene/scene.h"

#include <rendering/render_packet.h>
#include <rendering/render_scene.h>

#include "scene/model.h"
#include "scene/scene_node.h"

namespace midgard::scene {

renderer::RenderScene SceneGraph::generateRenderScene() {
    renderer::RenderScene rs;
    rs.m_camera = m_camera;

    for (auto& [model, transforms] : m_groups) {
        transforms.clear();
    }
    m_rootNode->collectTransforms(math::Mat4::identity(), m_groups);

    for (auto& [model, transforms] : m_groups) {

        if (transforms.size() >= 5) {
            render::InstancedDrawPacket packet;
            packet.mesh = model->getMesh();
            packet.material = model->getMaterial();
            packet.instanceTransforms = transforms;

            rs.m_instancedDraws.emplace_back(std::move(packet));
        } else {
            for (auto& transform : transforms) {
                render::SingleDrawPacket packet;
                packet.mesh = model->getMesh();
                packet.material = model->getMaterial();
                packet.worldTransform = transform;
                rs.m_singleDraws.emplace_back(packet);
            }
        }
    }

    return rs;
}

} // namespace midgard::scene