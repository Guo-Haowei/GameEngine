/// File: particle_kickoff.cs.hlsl
#include "cbuffer.hlsl.h"
#include "shader_resource_defines.hlsl.h"

[numthreads(1, 1, 1)] void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    int max_allocs = max(c_emitterMaxParticleCount - int(GlobalParticleCounter[0].aliveCount[c_preSimIdx]), 0);

    int emissionCount = min(c_particlesPerFrame, GlobalParticleCounter[0].deadCount);
    emissionCount = min(max_allocs, emissionCount);

    GlobalParticleCounter[0].emissionCount = emissionCount;
    GlobalParticleCounter[0].simulationCount = GlobalParticleCounter[0].aliveCount[c_preSimIdx] + GlobalParticleCounter[0].emissionCount;
    GlobalParticleCounter[0].aliveCount[c_postSimIdx] = 0;
}
