#include "particle_emitter_component.h"

#include "core/base/random.h"
#include "core/io/archive.h"

namespace my {

ParticleEmitterComponent::ParticleEmitterComponent() {
    m_maxParticleCount = 1000;
    m_emittedParticlesPerSecond = 100;
    m_particleScale = 0.02f;
    m_particleLifeSpan = 2.0f;
    m_startingVelocity = vec3(0.0f);
}

void ParticleEmitterComponent::Update(float p_elapsedTime, const vec3& p_position) {
    // @TODO: update properly
    unused(p_elapsedTime);
    unused(p_position);
}

void ParticleEmitterComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << m_maxParticleCount;
        p_archive << m_emittedParticlesPerSecond;
        p_archive << m_particleScale;
        p_archive << m_particleLifeSpan;
        p_archive << m_startingVelocity;
    } else {
        p_archive >> m_maxParticleCount;
        p_archive >> m_emittedParticlesPerSecond;
        p_archive >> m_particleScale;
        p_archive >> m_particleLifeSpan;
        p_archive >> m_startingVelocity;
    }
}

}  // namespace my
