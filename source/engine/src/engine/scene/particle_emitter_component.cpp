#include "particle_emitter_component.h"

#include "core/base/random.h"
#include "core/framework/event_queue.h"
#include "core/io/archive.h"

namespace my {

ParticleEmitterComponent::ParticleEmitterComponent() {
    maxParticleCount = 1000;
    particlesPerSec = 100;
    particleScale = 0.02f;
    particleLifeSpan = 2.0f;
    startingVelocity = vec3(0.0f);
}

void ParticleEmitterComponent::Update(float p_elapsedTime, const vec3& p_position) {
    unused(p_elapsedTime);
    unused(p_position);
}

void ParticleEmitterComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << maxParticleCount;
        p_archive << particlesPerSec;
        p_archive << particleScale;
        p_archive << particleLifeSpan;
        p_archive << startingVelocity;
    } else {
        p_archive >> maxParticleCount;
        p_archive >> particlesPerSec;
        p_archive >> particleScale;
        p_archive >> particleLifeSpan;
        p_archive >> startingVelocity;
    }
}

}  // namespace my
