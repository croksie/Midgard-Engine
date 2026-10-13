#pragma once
#include <memory>
#include <vector>

#include <math/math.h>


namespace midgard::scene {

class Model;

class SceneNode {
public:

    void setPosition(math::Vec3 position) { m_transform.position = position; }
    void setRotation(math::Vec3 rotation) { m_transform.rotation = rotation; }

    void addChild(std::shared_ptr<SceneNode> child) { m_childrens.push_back(child); }
    math::Mat4 getWorldMatrix() { return m_cachedWorldMatrix; }

    void collectDrawPackets();
private:

    math::Transform m_transform;
    std::shared_ptr<Model> m_model;
    math::Mat4 m_cachedWorldMatrix;
    bool m_isDirty;
    std::shared_ptr<SceneNode> m_parent;
    std::vector<std::shared_ptr<SceneNode>> m_childrens;

};


} // namespace midgard::scene