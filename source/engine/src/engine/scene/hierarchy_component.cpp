#include "hierarchy_component.h"

#include "core/io/archive.h"

namespace my {

void HierarchyComponent::serialize(Archive& p_archive, uint32_t) {
    m_parent_id.serialize(p_archive);
}

}  // namespace my