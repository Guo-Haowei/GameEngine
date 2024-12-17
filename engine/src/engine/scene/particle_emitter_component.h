#pragma once
#include "engine/core/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

namespace my {

class Archive;

struct ParticleEmitterComponent {
    void Update(float p_timestep);
    void Serialize(Archive& p_archive, uint32_t p_version);
    WARNING_PUSH()
    WARNING_DISABLE(4100, "-Wunused-parameter")
    bool Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const { return true; }
    bool Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) { return true; }
    WARNING_POP()
    uint32_t GetPreIndex() const { return aliveBufferIndex; }
    uint32_t GetPostIndex() const { return 1 - aliveBufferIndex; }

    bool gravity{ false };
    int maxParticleCount{ 1000 };
    int particlesPerFrame{ 10 };
    float particleScale{ 1.0f };
    float particleLifeSpan{ 3.0f };
    Vector3f startingVelocity{ 0.0f };

    // Non-Serialized
    std::shared_ptr<GpuStructuredBuffer> particleBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> counterBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> deadBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> aliveBuffer[2]{ nullptr, nullptr };

    uint32_t aliveBufferIndex{ 0 };
};

}  // namespace my
