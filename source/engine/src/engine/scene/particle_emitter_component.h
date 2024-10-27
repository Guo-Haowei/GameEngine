#pragma once
#include "core/math/geomath.h"

namespace my {

class Archive;

class ParticleEmitterComponent {
public:
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

    int m_emittedParticleCount{ 0 };
    int m_maxParticleCount{ 100 };
    int m_emittedParticlesPerFrame{ 10 };
    float m_particleScale{ 1.0f };
    float m_particleLifeSpan{ 3.0f };

    std::vector<Particle> m_particlePool;
};

}  // namespace my
