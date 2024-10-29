#include "particle_emitter_component.h"

#include "core/base/random.h"
#include "core/io/archive.h"

namespace my {

ParticleEmitterComponent::ParticleEmitterComponent() {
    m_emittedParticleCount = 0;
    m_maxParticleCount = 1000;
    m_emittedParticlesPerSecond = 100;
    m_particleScale = 0.02f;
    m_particleLifeSpan = 2.0f;
    m_startingVelocity = vec3(0.0f);
}

void ParticleEmitterComponent::Update(float p_elapsedTime, const vec3& p_position) {
    // resize pool if necessary
    ResizePool(m_maxParticleCount);

    // emit particles
    const int particle_emit_per_frame = static_cast<int>(m_emittedParticlesPerSecond * p_elapsedTime);
    int particle_to_emit = glm::min(particle_emit_per_frame, m_maxParticleCount - m_emittedParticleCount);
    particle_to_emit = glm::max(particle_to_emit, 0);

    std::vector<Particle*> free_slots;
    free_slots.reserve(particle_to_emit);

    // update old particles
    for (size_t particle_idx = 0; particle_idx < m_particlePool.size(); ++particle_idx) {
        Particle& particle = m_particlePool[particle_idx];
        if (particle.lifeRemaining <= 0.0f && particle.isActive) {
            --m_emittedParticleCount;
            particle.isActive = false;
        }

        if (!particle.isActive) {
            free_slots.push_back(&particle);
            continue;
        }

        particle.lifeRemaining -= p_elapsedTime;
        particle.position += p_elapsedTime * particle.velocity;
    }

    // emit new particles
    DEV_ASSERT(particle_to_emit <= free_slots.size());
    for (int i = 0; i < particle_to_emit; ++i) {
        Particle& particle = *free_slots[i];
        particle.position = p_position;
        particle.lifeSpan = m_particleLifeSpan;
        particle.lifeRemaining = m_particleLifeSpan;
        particle.isActive = true;

        particle.velocity = m_startingVelocity;
        particle.velocity.x += Random::Float() - 0.5f;
        particle.velocity.y += Random::Float() - 0.5f;
        particle.velocity.z += Random::Float() - 0.5f;
    }

    m_emittedParticleCount += particle_to_emit;
}

void ParticleEmitterComponent::ResizePool(uint32_t p_size) {
    const uint32_t pool_size = static_cast<uint32_t>(m_particlePool.size());

    if (p_size > pool_size) {
        m_particlePool.resize(p_size);
    }
}

void ParticleEmitterComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);
    CRASH_NOW();
    if (p_archive.IsWriteMode()) {
    } else {
    }
}

}  // namespace my
