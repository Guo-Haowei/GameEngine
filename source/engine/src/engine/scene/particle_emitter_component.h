#pragma once
#include "core/math/geomath.h"
#include "rendering/gpu_resource.h"

namespace my {

class Archive;

struct ParticleEmitterComponent {
    void Update(float p_elapsedTime, const vec3& p_position);
    void Serialize(Archive& p_archive, uint32_t p_version);
    uint32_t GetPreIndex() const { return aliveBufferIndex; }
    uint32_t GetPostIndex() const { return 1 - aliveBufferIndex; }

    int maxParticleCount{ 1000 };
    int particlesPerFrame{ 100 };
    float particleScale{ 1.0f };
    float particleLifeSpan{ 3.0f };
    vec3 startingVelocity{ 0.0f };

    // Non-Serialized
    std::shared_ptr<GpuStructuredBuffer> particleBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> counterBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> deadBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> aliveBuffer[2]{ nullptr, nullptr };

    uint32_t aliveBufferIndex{ 0 };
};

}  // namespace my
