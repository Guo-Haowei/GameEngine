#pragma once
#include "engine/core/math/geomath.h"
#include "engine/core/systems/entity.h"

namespace my {

class Archive;

struct ArmatureComponent {
    enum FLAGS {
        NONE = 0,
    };
    uint32_t flags = NONE;

    std::vector<ecs::Entity> boneCollection;
    std::vector<Matrix4x4f> inverseBindMatrices;

    // Non-Serialized
    std::vector<Matrix4x4f> boneTransforms;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my
