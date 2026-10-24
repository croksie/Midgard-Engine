#pragma once
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace midgard::bifrost {

class Buffer;
class Texture;

struct BindGroupEntry {
    uint32_t binding = 0;
    std::shared_ptr<Buffer> buffer = nullptr;
    std::shared_ptr<Texture> texture = nullptr;
};

struct BindGroupDesc {
    std::vector<BindGroupEntry> entries;
};

class BindGroup {
public:
    explicit BindGroup(BindGroupDesc desc) : m_desc(std::move(desc)) {}
    virtual ~BindGroup() = default;

    [[nodiscard]] const BindGroupDesc& getDesc() const { return m_desc; }

protected:
    BindGroupDesc m_desc;
};

} // namespace midgard::bifrost
