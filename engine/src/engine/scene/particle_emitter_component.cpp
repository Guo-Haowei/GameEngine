#include "particle_emitter_component.h"

#include "engine/core/base/random.h"
#include "engine/core/framework/event_queue.h"
#include "engine/core/io/archive.h"

namespace my {

void ParticleEmitterComponent::Update(float) {
    aliveBufferIndex = 1 - aliveBufferIndex;
}

void ParticleEmitterComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << maxParticleCount;
        p_archive << particlesPerFrame;
        p_archive << particleScale;
        p_archive << particleLifeSpan;
        p_archive << startingVelocity;
        p_archive << gravity;
    } else {
        p_archive >> maxParticleCount;
        p_archive >> particlesPerFrame;
        p_archive >> particleScale;
        p_archive >> particleLifeSpan;
        p_archive >> startingVelocity;
        if (p_version >= 9) {
            p_archive >> gravity;
        }
    }
}

}  // namespace my
