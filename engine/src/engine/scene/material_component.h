#pragma once
#include "engine/core/math/geomath.h"

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

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
    };

    float metallic = 0.0f;
    float roughness = 1.0f;
    float emissive = 0.0f;
    Vector4f baseColor = Vector4f(1);
    TextureMap textures[TEXTURE_MAX];
    bool useTexures;

    void Serialize(Archive& p_archive, uint32_t p_version);
    WARNING_PUSH()
    WARNING_DISABLE(4100, "-Wunused-parameter")
    bool Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const { return true; }
    bool Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) { return true; }
    WARNING_POP()
};

}  // namespace my
