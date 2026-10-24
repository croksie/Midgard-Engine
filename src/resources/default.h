#pragma once
#include <memory>

#include "bifrost/texture.h"

namespace midgard::bifrost {
    class Bifrost; 
}

namespace midgard::resource {

class DefaultResources {
public:
    static void init(bifrost::Bifrost* bifrost);
    static void shutdown();

    static std::shared_ptr<bifrost::Texture> getDefaultTexture() { return s_defaultTexture; }
    static std::shared_ptr<bifrost::Texture> getDefaultNormalTexture() { return s_defaultNormalTexture ? s_defaultNormalTexture : s_defaultTexture; }
    static std::shared_ptr<bifrost::Texture> getDefaultBlackTexture() { return s_defaultBlackTexture ? s_defaultBlackTexture : s_defaultTexture; }

private:
    inline static std::shared_ptr<bifrost::Texture> s_defaultTexture = nullptr;
    inline static std::shared_ptr<bifrost::Texture> s_defaultNormalTexture = nullptr;
    inline static std::shared_ptr<bifrost::Texture> s_defaultBlackTexture = nullptr;
};

} // namespace midgard::resource