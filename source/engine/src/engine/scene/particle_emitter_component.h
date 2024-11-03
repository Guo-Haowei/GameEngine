#pragma once
#include "core/math/geomath.h"
#include "rendering/gpu_resource.h"

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

    // Non-Serialized
    std::shared_ptr<GpuStructuredBuffer> particleBuffer;
    std::shared_ptr<GpuStructuredBuffer> counterBuffer;
    std::shared_ptr<GpuStructuredBuffer> deadBuffer;
    std::shared_ptr<GpuStructuredBuffer> aliveBuffer[2]{ nullptr, nullptr };
};

}  // namespace my
