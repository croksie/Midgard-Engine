#pragma once
#include <memory>
#include <vector>

#include <math/math.h>

namespace midgard::scene {

class Model;

class SceneNode {
public:
    SceneNode() = default;
    SceneNode(std::shared_ptr<scene::Model> model) : m_model(model) {}

    void setPosition(math::Vec3 position) {
        m_transform.position = position;
        setDirty();
    }
    void setRotation(math::Vec3 rotation) {
        m_transform.rotation = rotation;
        setDirty();
    }

    void addChild(std::shared_ptr<SceneNode> child) { m_childrens.push_back(child); }
    math::Mat4 getWorldMatrix() const { return m_cachedWorldMatrix; }

    void setDirty() {
        if (m_isDirty)
            return;
        m_isDirty = true;
        for (auto& child : m_childrens) {
            child->setDirty();
        }
    }
    void collectTransforms(const math::Mat4& parentMatrix,
                           std::unordered_map<Model*, std::vector<math::Mat4>>& groups) {
        if (m_isDirty) {
            m_cachedWorldMatrix = parentMatrix * m_transform.toMatrix();
            m_isDirty = false;
        }
        if (m_model) {
            groups[m_model.get()].push_back(m_cachedWorldMatrix);
        }
        for (auto& child : m_childrens) {
            child->collectTransforms(m_cachedWorldMatrix, groups);
        }
    }

private:
    math::Transform m_transform;
    math::Mat4 m_cachedWorldMatrix = math::Mat4::identity();
    bool m_isDirty = true;

    std::shared_ptr<SceneNode> m_parent;
    std::vector<std::shared_ptr<SceneNode>> m_childrens;
    std::shared_ptr<Model> m_model;
};

} // namespace midgard::scene