#include "name_component.h"

#include "core/io/archive.h"

namespace my {

void NameComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << m_name;
    } else {
        archive >> m_name;
    }
}

}  // namespace my
