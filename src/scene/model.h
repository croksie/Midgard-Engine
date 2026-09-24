#pragma once
#include <memory>

#include "math/math.h"

namespace midgard::resource {
    class Mesh;
    class Material;
}

namespace midgard::scene {

class Model {
public:
    Model() = default;
    Model(std::shared_ptr<resource::Mesh> mesh, std::shared_ptr<resource::Material> material) 
        : m_mesh(mesh), m_material(material) {}

    std::shared_ptr<resource::Mesh> getMesh() const { return m_mesh; }
    std::shared_ptr<resource::Material> getMaterial() const { return m_material; }

    void setMesh(std::shared_ptr<resource::Mesh> mesh) { m_mesh = mesh; }
    void setMaterial(std::shared_ptr<resource::Material> material) { m_material = material; }

    math::Vec3 getPosition() const { return m_position; }
    void setPosition(math::Vec3 position) { m_position = position; }

    math::Mat4 getTransform() const;
    
private:
    std::shared_ptr<resource::Mesh> m_mesh;
    std::shared_ptr<resource::Material> m_material;

    
    math::Vec3 m_position;
};

} // namespace midgard::scene