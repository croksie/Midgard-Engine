#pragma once
#include <memory>
#include <vector>

#include <math/transform.h>

#include <model.h>


namespace midgard::scene{


class ModelInstance {

public:

    math::Transform getTransform() const { return m_transform; }
    math::Vec3 getPosition() const { return m_transform.position; }
    math::Vec3 getRotation() const { return m_transform.rotation; }
    math::Vec3 getScale() const { return m_transform.scale; }

    void setTransform(math::Transform transform) { m_transform = transform; }
    void setPosition(const math::Vec3& position) { m_transform.position = position; }
    void setPosition(const math::Vec3& rotation) { m_transform.rotation = rotation; }
    void setPosition(const math::Vec3& scale) { m_transform.scale = scale; }

private:
    std::shared_ptr<scene::Model> m_model;
    std::vector<std::shared_ptr<ModelInstance>> childs;

    math::Transform m_transform;


};

} // namespace midgard::scene
