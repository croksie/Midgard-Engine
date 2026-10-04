#pragma once
#include <unordered_map>

#include <scene/model.h>
#include <scene/camera.h>


namespace midgard::renderer {

class RenderScene {
public:



    scene::Camera getCamera() { return m_camera; }

private:
    std::unordered_map<uint32_t, scene::Model> m_models;
    scene::Camera m_camera;

};

} // namepace midgard::renderer