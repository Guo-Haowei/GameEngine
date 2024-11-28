#include "collider_component.h"

#include "engine/core/io/archive.h"

namespace my {

void BoxColliderComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << box;
    } else {
        p_archive >> box;
    }
}

void MeshColliderComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << objectId;
    } else {
        p_archive >> objectId;
    }
}

}  // namespace my