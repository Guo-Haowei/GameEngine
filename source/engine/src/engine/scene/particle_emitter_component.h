#pragma once
#include "core/math/geomath.h"

namespace my {

class Archive;

class ParticleEmitterComponent {
public:
    ParticleEmitterComponent();

    void Update(float p_elapsedTime, const vec3& p_position);

    float GetParticleScale() const { return m_particleScale; }
    float GetParticleLifeSpan() const { return m_particleLifeSpan; }

    vec3& GetStartingVelocityRef() { return m_startingVelocity; }
    const vec3& GetStartingVelocity() const { return m_startingVelocity; }

    int& GetMaxParticleCountRef() { return m_maxParticleCount; }
    int& GetEmittedParticlesPerSecondRef() { return m_emittedParticlesPerSecond; }
    float& GetParticleScaleRef() { return m_particleScale; }
    float& GetParticleLifeSpanRef() { return m_particleLifeSpan; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    int m_maxParticleCount;
    int m_emittedParticlesPerSecond;
    float m_particleScale;
    float m_particleLifeSpan;
    vec3 m_startingVelocity;
};

}  // namespace my
