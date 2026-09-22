#include "resources/default.h"

#include "bifrost/bifrost.h"

namespace midgard::resource {

void DefaultResources::init(bifrost::Bifrost* bifrost) {
    uint32_t white = 0xFFFFFFFF;
    bifrost::TextureDesc texDesc{};
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = bifrost::TextureFormat::RGBA8_SRGB;
    texDesc.initialData = &white;
    texDesc.size = sizeof(white);
    s_defaultTexture = bifrost->createTexture(texDesc);
}

void DefaultResources::shutdown() {
    s_defaultTexture.reset();
}

} // namespace midgard::resource