#include "collider_component.h"

#include "core/io/archive.h"

namespace my {

void BoxColliderComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.isWriteMode()) {
        p_archive << box;
    } else {
        p_archive >> box;
    }
}

void MeshColliderComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.isWriteMode()) {
        p_archive << object_id;
    } else {
        p_archive >> object_id;
    }
}

}  // namespace my