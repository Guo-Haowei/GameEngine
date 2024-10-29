#pragma once
#include "core/math/geomath.h"

namespace my {

class Archive;

class ParticleEmitterComponent {
public:
    struct Particle {
        vec3 position;
        vec3 velocity;
        vec4 colorBegine;
        vec4 colorEnd;

        float lifeSpan;
        float lifeRemaining;
        bool isActive = false;
    };

    using ParticlePool = std::vector<Particle>;

    ParticleEmitterComponent();

    void Update(float p_elapsedTime, const vec3& p_position);

    float GetParticleScale() const { return m_particleScale; }
    float GetParticleLifeSpan() const { return m_particleLifeSpan; }

    vec3& GetStartingVelocityRef() { return m_startingVelocity; }
    int& GetMaxParticleCountRef() { return m_maxParticleCount; }
    int& GetEmittedParticlesPerSecondRef() { return m_emittedParticlesPerSecond; }
    float& GetParticleScaleRef() { return m_particleScale; }
    float& GetParticleLifeSpanRef() { return m_particleLifeSpan; }
    ParticlePool& GetParticlePoolRef() { return m_particlePool; }
    const ParticlePool& GetParticlePoolRef() const { return m_particlePool; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    void ResizePool(uint32_t p_size);

    int m_maxParticleCount;
    int m_emittedParticlesPerSecond;
    float m_particleScale;
    float m_particleLifeSpan;
    vec3 m_startingVelocity;

    // Non-Serialized
    int m_emittedParticleCount;
    ParticlePool m_particlePool;
};

}  // namespace my
