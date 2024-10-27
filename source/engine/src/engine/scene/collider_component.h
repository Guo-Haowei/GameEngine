#pragma once
#include "core/math/aabb.h"
#include "core/systems/entity.h"

namespace my {

class Archive;

struct BoxColliderComponent {
    AABB box;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

struct MeshColliderComponent {
    ecs::Entity object_id;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my
