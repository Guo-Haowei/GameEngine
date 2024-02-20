#include "collider_component.h"

#include "core/io/archive.h"

namespace my {

void SelectableComponent::serialize(Archive& archive, uint32_t version) {
    unused(version);

    // nothing to do
    if (archive.is_write_mode()) {
    } else {
    }
}

void BoxColliderComponent::serialize(Archive& archive, uint32_t version) {
    unused(version);

    if (archive.is_write_mode()) {
        archive << box;
    } else {
        archive >> box;
    }
}

void MeshColliderComponent::serialize(Archive& archive, uint32_t version) {
    unused(version);

    if (archive.is_write_mode()) {
        archive << object_id;
    } else {
        archive >> object_id;
    }
}

}  // namespace my