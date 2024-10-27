#pragma once
#include "core/systems/entity.h"

namespace my {

class Archive;

class HierarchyComponent {
public:
    ecs::Entity GetParent() const { return m_parentId; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    ecs::Entity m_parentId;

    friend class Scene;
};

}  // namespace my
