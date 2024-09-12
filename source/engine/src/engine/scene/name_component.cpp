#include "name_component.h"

#include "core/io/archive.h"

namespace my {

void NameComponent::serialize(Archive& archive, uint32_t) {
    if (archive.isWriteMode()) {
        archive << m_name;
    } else {
        archive >> m_name;
    }
}

}  // namespace my
