#pragma once
#include "core/math/geomath.h"
#include "core/systems/entity.h"

namespace my {

class Archive;

struct ArmatureComponent {
    enum FLAGS {
        NONE = 0,
    };
    uint32_t flags = NONE;

    std::vector<ecs::Entity> boneCollection;
    std::vector<mat4> inverseBindMatrices;

    // Non-Serialized
    std::vector<mat4> boneTransforms;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my
