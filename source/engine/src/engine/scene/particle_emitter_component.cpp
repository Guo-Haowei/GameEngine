#include "particle_emitter_component.h"

#include <random>

namespace my {

// @TODO: refactor
class Random {
public:
    static void Initialize() {
        s_randomEngine.seed(std::random_device()());
    }

    static float Float() {
        return s_distribution(s_randomEngine);
    }

private:
    static std::mt19937 s_randomEngine;
    static std::uniform_real_distribution<float> s_distribution;
};

std::mt19937 Random::s_randomEngine;
std::uniform_real_distribution<float> Random::s_distribution(0.0f, 1.0f);

ParticleEmitterComponent::ParticleEmitterComponent() {
    m_emittedParticleCount = 0;
    m_maxParticleCount = 1000;
    m_emittedParticlesPerSecond = 100;
    m_particleScale = 0.02f;
    m_particleLifeSpan = 2.0f;
    m_startingVelocity = vec3(0.0f);
}

void ParticleEmitterComponent::Update(float p_elapsedTime) {
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
        particle.position = vec3(0.0f);
        particle.velocity = vec3(0.0f);
        particle.lifeSpan = m_particleLifeSpan;
        particle.lifeRemaining = m_particleLifeSpan;
        particle.isActive = true;

        particle.velocity.x += Random::Float() - 0.5f;
        particle.velocity.y += Random::Float() - 0.5f;
        particle.velocity.z += Random::Float() - 0.5f;
        particle.velocity += m_startingVelocity;
    }

    m_emittedParticleCount += particle_to_emit;
}

void ParticleEmitterComponent::ResizePool(uint32_t p_size) {
    const uint32_t pool_size = static_cast<uint32_t>(m_particlePool.size());

    if (p_size > pool_size) {
        m_particlePool.resize(p_size);
    }
}

}  // namespace my
