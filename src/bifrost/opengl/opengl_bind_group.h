#pragma once

#include "bifrost/bind_group.h"

namespace midgard::bifrost::opengl {

class OpenGLBindGroup : public BindGroup {
public:
    explicit OpenGLBindGroup(BindGroupDesc desc) : BindGroup(std::move(desc)) {}
    ~OpenGLBindGroup() override = default;
};

} // namespace midgard::bifrost::opengl
