#pragma once
#include <memory>
#include <vector>

#include <math/math.h>

namespace midgard::scene {

class Model;

class SceneNode {
public:
    SceneNode() = default;
    explicit SceneNode(std::shared_ptr<Model> model) {
        addModel(std::move(model));
    }

    void setPosition(math::Vec3 position) {
        m_transform.position = position;
        setDirty();
    }
    void setRotation(math::Vec3 rotation) {
        m_transform.rotation = rotation;
        setDirty();
    }
    void setScale(math::Vec3 scale) {
        m_transform.scale = scale;
        setDirty();
    }

    void setTransform(math::Transform transform) {
        m_transform = transform;
        setDirty();
    }

    void addModel(std::shared_ptr<Model> model) { m_models.emplace_back(std::move(model)); }

    void addChild(std::shared_ptr<SceneNode> child) {
        if (!child) return;
        child->m_parent = this;
        child->setDirty();
        m_childrens.push_back(std::move(child));
    }
    SceneNode* getParent() const { return m_parent; }
    math::Mat4 getWorldMatrix() const { return m_cachedWorldMatrix; }

    void setDirty() {
        m_isDirty = true;
        for (auto& child : m_childrens) {
            if (child) {
                child->setDirty();
            }
        }
    }
    void collectTransforms(const math::Mat4& parentMatrix, std::unordered_map<Model*, std::vector<math::Mat4>>& groups) {
        if (m_isDirty) {
            m_cachedWorldMatrix = parentMatrix * m_transform.toMatrix();
            m_isDirty = false;
        }
        if (m_models.size() > 0) {
            for (auto model : m_models) {
                groups[model.get()].push_back(m_cachedWorldMatrix);
            }
        }
        for (auto& child : m_childrens) {
            child->collectTransforms(m_cachedWorldMatrix, groups);
        }
    }

private:
    math::Transform m_transform;
    math::Mat4 m_cachedWorldMatrix = math::Mat4::identity();
    bool m_isDirty = true;

    SceneNode* m_parent = nullptr;
    std::vector<std::shared_ptr<SceneNode>> m_childrens;
    std::vector<std::shared_ptr<Model>> m_models;
};

} // namespace midgard::scene