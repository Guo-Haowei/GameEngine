#include "armature_component.h"

#include "core/io/archive.h"

namespace my {

void ArmatureComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.isWriteMode()) {
        p_archive << flags;
        p_archive << bone_collection;
        p_archive << inverse_bind_matrices;
    } else {
        p_archive >> flags;
        p_archive >> bone_collection;
        p_archive >> inverse_bind_matrices;
    }
}

}  // namespace my
