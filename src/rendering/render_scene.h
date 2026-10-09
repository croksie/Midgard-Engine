#pragma once
#include <vector>

#include <scene/camera.h>
#include <rendering/render_packet.h>


namespace midgard::renderer {

class RenderScene {
public:
    scene::Camera getCamera() { return m_camera; }

private:
    std::vector<render::SingleDrawPacket> m_singleDraws;
    std::vector<render::InstancedDrawPacket> m_intancedDraws;
    scene::Camera m_camera;

};

} // namepace midgard::renderer