#include "scene/model.h"


namespace midgard::scene {


math::Mat4 midgard::scene::Model::getTransform() const
{
    math::Mat4 modelMat = math::Mat4(1.0f);
    modelMat[3] = math::Vec4(getPosition(), 1.0f);
    return modelMat;
}

} // namespace midgard::scene
