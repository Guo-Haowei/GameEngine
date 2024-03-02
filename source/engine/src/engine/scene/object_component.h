#pragma once
#include "core/systems/entity.h"

namespace my {

class Archive;

struct ObjectComponent {
    enum : uint32_t {
        NONE = BIT(0),
        RENDERABLE = BIT(1),
        CAST_SHADOW = BIT(2),
    };

    uint32_t flags = RENDERABLE | CAST_SHADOW;
    ecs::Entity mesh_id;

    void serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my
