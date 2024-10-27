#include "hierarchy_component.h"

#include "core/io/archive.h"

namespace my {

void HierarchyComponent::Serialize(Archive& p_archive, uint32_t) {
    m_parentId.Serialize(p_archive);
}

}  // namespace my