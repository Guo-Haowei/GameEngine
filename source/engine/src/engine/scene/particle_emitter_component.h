#pragma once
#include "core/math/geomath.h"

namespace my {

class Archive;

struct ParticleEmitterComponent {
    ParticleEmitterComponent();

    void Update(float p_elapsedTime, const vec3& p_position);

    void Serialize(Archive& p_archive, uint32_t p_version);

    int maxParticleCount;
    int particlesPerSec;
    float particleScale;
    float particleLifeSpan;
    vec3 startingVelocity;
};

}  // namespace my
