/// File: particle_initialization.cs.hlsl
#include "cbuffer.hlsl.h"
#include "shader_resource_defines.hlsl.h"

[numthreads(PARTICLE_LOCAL_SIZE, 1, 1)] void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    const uint index = dispatch_thread_id.x;

    // @TODO: consider remove this branching
    if (index == 0) {
        GlobalParticleCounter[0].deadCount = MAX_PARTICLE_COUNT;
        GlobalParticleCounter[0].aliveCount[0] = 0;
        GlobalParticleCounter[0].aliveCount[1] = 0;
    }

    if (index < MAX_PARTICLE_COUNT) {
        // dead particles
        GlobalDeadIndices[index] = index;
    }
}
