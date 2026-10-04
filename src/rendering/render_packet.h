#pragma once
#include <memory>
#include <vector>

#include <math/mat4.h>

namespace midgard::resource {
    class Mesh;
    class Material;
}

namespace midgard::bifrost {
    class Buffer;
}

namespace midgard::render {

struct SingleDrawPacket {
    std::shared_ptr<resource::Mesh> mesh;
    std::shared_ptr<resource::Material> material;
    math::Mat4 worldTransform; // 64 octets DEDANS !
};

struct InstancedDrawPacket {
    std::shared_ptr<resource::Mesh> mesh;
    std::shared_ptr<resource::Material> material;
    std::shared_ptr<bifrost::Buffer> instanceBuffer;
    uint32_t instanceCount;
};

} // namespace midgard::render