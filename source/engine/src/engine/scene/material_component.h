#pragma once
#include "assets/image.h"
#include "core/math/geomath.h"

namespace my {

class Archive;

struct MaterialComponent {
    enum {
        TEXTURE_BASE,
        TEXTURE_NORMAL,
        TEXTURE_METALLIC_ROUGHNESS,
        TEXTURE_MAX,
    };

    struct TextureMap {
        std::string path;
        // Non-serialized
        bool enabled = true;
        ImageHandle* image = nullptr;
    };

    float metallic = 0.0f;
    float roughness = 1.0f;
    float emissive = 0.0f;
    Vector4f baseColor = Vector4f(1);
    TextureMap textures[TEXTURE_MAX];
    bool useTexures;

    // @TODO: remove this, this is poorly designed
    void RequestImage(int p_slot, const std::string& p_path);

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my
