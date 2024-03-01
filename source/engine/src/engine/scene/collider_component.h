#pragma once
#include "core/math/aabb.h"
#include "core/systems/entity.h"

namespace my {

class Archive;

// @TODO: remove this
struct SelectableComponent {
    bool selected = false;

    void serialize(Archive& archive, uint32_t version);
};

struct BoxColliderComponent {
    AABB box;

    void serialize(Archive& archive, uint32_t version);
};

struct MeshColliderComponent {
    ecs::Entity object_id;

    void serialize(Archive& archive, uint32_t version);
};

}  // namespace my
