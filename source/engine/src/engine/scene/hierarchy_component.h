#pragma once
#include "core/systems/entity.h"

namespace my {

class Archive;

class HierarchyComponent {
public:
    ecs::Entity GetParent() const { return m_parent_id; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    ecs::Entity m_parent_id;

    friend class Scene;
};

}  // namespace my
