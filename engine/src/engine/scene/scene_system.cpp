#include "scene_system.h"

namespace my {

void UpdateMeshEmitter(float p_timestep,
                       const TransformComponent& p_transform,
                       MeshEmitterComponent& p_emitter) {
    if (p_emitter.flags & MeshEmitterComponent::PAUSED) {
        return;
    }

    // 1. emit new particles
    const int emission_count = math::max(p_emitter.emissionPerFrame, (int)p_emitter.deadList.size());
    p_emitter.aliveList.reserve(p_emitter.aliveList.size() + emission_count);
    const auto& position = p_transform.GetTranslation();
    for (int i = 0; i < emission_count; ++i) {
        const uint32_t free_index = p_emitter.deadList[free_index];
        p_emitter.deadList.pop_back();
        p_emitter.aliveList.push_back(free_index);

        p_emitter.InitParticle(free_index, position);
    }

    // 2. update alive ones (TODO: use jobsystem)
    for (const uint32_t index : p_emitter.aliveList) {
        p_emitter.UpdateParticle(index, p_timestep);
    }

    // 3. recycle
    if (p_emitter.flags & MeshEmitterComponent::RECYCLE) {
        LOG_WARN("TODO");
    }
}

}  // namespace my
