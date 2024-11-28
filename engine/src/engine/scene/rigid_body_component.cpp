#include "rigid_body_component.h"

#include "engine/core/io/archive.h"

namespace my {

void RigidBodyComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << shape;
        p_archive << param;
        p_archive << mass;
    } else {
        p_archive >> shape;
        p_archive >> param;
        p_archive >> mass;
    }
}

}  // namespace my
