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

    uint8_t normalPixel[4] = {128, 128, 255, 255};
    bifrost::TextureDesc normalDesc{};
    normalDesc.width = 1;
    normalDesc.height = 1;
    normalDesc.format = bifrost::TextureFormat::RGBA8_UNORM;
    normalDesc.initialData = normalPixel;
    normalDesc.size = sizeof(normalPixel);
    s_defaultNormalTexture = bifrost->createTexture(normalDesc);

    uint8_t blackPixel[4] = {0, 0, 0, 255};
    bifrost::TextureDesc blackDesc{};
    blackDesc.width = 1;
    blackDesc.height = 1;
    blackDesc.format = bifrost::TextureFormat::RGBA8_SRGB;
    blackDesc.initialData = blackPixel;
    blackDesc.size = sizeof(blackPixel);
    s_defaultBlackTexture = bifrost->createTexture(blackDesc);
}

void DefaultResources::shutdown() {
    s_defaultTexture.reset();
    s_defaultNormalTexture.reset();
    s_defaultBlackTexture.reset();
}

} // namespace midgard::resource