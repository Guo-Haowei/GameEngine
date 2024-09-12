#include "object_component.h"

#include "core/io/archive.h"

namespace my {

void ObjectComponent::serialize(Archive& p_archive, uint32_t) {
    if (p_archive.isWriteMode()) {
        p_archive << flags;
        p_archive << mesh_id;
    } else {
        p_archive >> flags;
        p_archive >> mesh_id;
    }
}

}  // namespace my
