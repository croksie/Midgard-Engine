#include "scene/scene.h"

namespace midgard::scene {

void SceneGraph::addModel(const Model &model)
{
    m_models.push_back(model);
}

} // namespace midgard::scene