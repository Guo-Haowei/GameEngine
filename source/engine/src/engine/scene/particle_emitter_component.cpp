#include "particle_emitter_component.h"

#include "core/base/random.h"
#include "core/framework/event_queue.h"
#include "core/io/archive.h"

namespace my {

void ParticleEmitterComponent::Update(float p_elapsedTime) {
    unused(p_elapsedTime);

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
    } else {
        p_archive >> maxParticleCount;
        p_archive >> particlesPerFrame;
        p_archive >> particleScale;
        p_archive >> particleLifeSpan;
        p_archive >> startingVelocity;
    }
}

}  // namespace my
