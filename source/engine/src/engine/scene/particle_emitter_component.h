#pragma once
#include "core/math/geomath.h"

namespace my {

class Archive;

class ParticleEmitterComponent {
public:
    ParticleEmitterComponent();

    void Update(float p_elapsedTime);

private:
    void ResizePool(uint32_t p_size);

    struct Particle {
        vec3 position;
        vec3 velocity;
        vec4 colorBegine;
        vec4 colorEnd;

        float lifeSpan;
        float lifeRemaining;
        bool isActive = false;
    };

    int m_emittedParticleCount;
    int m_maxParticleCount;
    int m_emittedParticlesPerFrame;
    float m_particleScale;
    float m_particleLifeSpan;

    std::vector<Particle> m_particlePool;

    friend class GraphicsManager;
};

}  // namespace my
