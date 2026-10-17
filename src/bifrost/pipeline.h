#pragma once
#include <cstdint>
#include <memory>

namespace midgard::bifrost {
class Buffer;
class Shader;
class Texture;

struct PipelineInfo {
    std::shared_ptr<Shader> vertexShader;
    std::shared_ptr<Shader> fragmentShader;
    bool useInstance = false;
};

class Pipeline {
public:
    virtual ~Pipeline() = default;

    virtual void bindVertexBuffer(std::shared_ptr<Buffer> vertexBuffer) = 0;
    virtual void bindInstanceBuffer(std::shared_ptr<Buffer> instanceBuffer, uint32_t offset) = 0;
    virtual void bindIndexBuffer(std::shared_ptr<Buffer> indexBuffer) = 0;
    virtual void bindTexture(std::shared_ptr<Texture> texture, uint32_t slot = 0) = 0;
};

} // namespace midgard::bifrost
