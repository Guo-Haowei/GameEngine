#include "force_field_component.h"

#include "engine/core/io/archive.h"

namespace my {

void ForceFieldComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << strength;
        p_archive << radius;
    } else {
        p_archive >> strength;
        p_archive >> radius;
    }
}

}  // namespace my
