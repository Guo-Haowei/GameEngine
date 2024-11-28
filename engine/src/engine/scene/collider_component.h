#pragma once
#include "engine/core/math/aabb.h"
#include "engine/core/systems/entity.h"

namespace my {

class Archive;

struct BoxColliderComponent {
    AABB box;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

struct MeshColliderComponent {
    ecs::Entity objectId;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my
