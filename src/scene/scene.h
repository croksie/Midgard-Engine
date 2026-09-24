#pragma once
#include <vector>

#include "scene/model.h"

namespace midgard::scene {

class Model;

class SceneGraph {
public:

    const std::vector<Model>& getModels() const { return m_models; }

    void addModel(const Model& model);

private:

    std::vector<Model> m_models;
};


} // namespace midgard::scene