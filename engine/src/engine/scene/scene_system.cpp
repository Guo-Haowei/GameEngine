#include "scene_system.h"

namespace my {

void UpdateMeshEmitter(float p_timestep,
                       const TransformComponent& p_transform,
                       MeshEmitterComponent& p_emitter) {
    if (!p_emitter.IsRunning()) {
        return;
    }

    // 1. emit new particles
    const int emission_count = math::min(p_emitter.emissionPerFrame, (int)p_emitter.deadList.size());
    p_emitter.aliveList.reserve(p_emitter.aliveList.size() + emission_count);
    const auto& position = p_transform.GetTranslation();
    for (int i = 0; i < emission_count; ++i) {
        DEV_ASSERT(!p_emitter.deadList.empty());
        const auto free_index = p_emitter.deadList.back();
        p_emitter.deadList.pop_back();
        p_emitter.aliveList.push_back(free_index);

        p_emitter.InitParticle(free_index, position);
    }

    // 2. update alive ones (TODO: use jobsystem)
    for (const auto index : p_emitter.aliveList) {
        p_emitter.UpdateParticle(index, p_timestep);
    }

    // 3. recycle
    std::vector<MeshEmitterComponent::Index> tmp;
    tmp.reserve(p_emitter.aliveList.size());
    const bool recycle = p_emitter.IsRecycle();
    for (int i = (int)p_emitter.aliveList.size() - 1; i >= 0; --i) {
        const auto index = p_emitter.aliveList[i];
        auto& p = p_emitter.particles[index.v];
        if (p.lifespan <= 0.0f) {
            p_emitter.aliveList.pop_back();
            if (recycle) {
                p_emitter.deadList.push_back(index);
            }
        } else {
            tmp.push_back(index);
        }
    }
    p_emitter.aliveList = std::move(tmp);
}

}  // namespace my
