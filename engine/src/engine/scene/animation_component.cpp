#include "animation_component.h"

#include "engine/core/io/archive.h"

namespace my {

void AnimationComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << start;
        p_archive << end;
        p_archive << timer;
        p_archive << amount;
        p_archive << speed;
        p_archive << channels;

        uint64_t num_samplers = samplers.size();
        p_archive << num_samplers;
        for (uint64_t i = 0; i < num_samplers; ++i) {
            p_archive << samplers[i].keyframeTmes;
            p_archive << samplers[i].keyframeData;
        }
    } else {
        p_archive >> flags;
        p_archive >> start;
        p_archive >> end;
        p_archive >> timer;
        p_archive >> amount;
        p_archive >> speed;
        p_archive >> channels;

        uint64_t num_samplers = 0;
        p_archive >> num_samplers;
        samplers.resize(num_samplers);
        for (uint64_t i = 0; i < num_samplers; ++i) {
            p_archive >> samplers[i].keyframeTmes;
            p_archive >> samplers[i].keyframeData;
        }
    }
}

}  // namespace my
