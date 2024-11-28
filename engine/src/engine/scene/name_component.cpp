#include "name_component.h"

#include "engine/core/io/archive.h"

namespace my {

void NameComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_name;
    } else {
        p_archive >> m_name;
    }
}

}  // namespace my
