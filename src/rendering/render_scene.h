#pragma once
#include <vector>

#include <rendering/render_packet.h>
#include <scene/camera.h>

namespace midgard::renderer {

struct RenderScene {
    std::vector<render::SingleDrawPacket> m_singleDraws;
    std::vector<render::InstancedDrawPacket> m_instancedDraws;
    scene::Camera m_camera;
};

} // namespace midgard::renderer