#include "cbuffer.h"
#include "particle_defines.h"

[numthreads(PARTICLE_LOCAL_SIZE, 1, 1)] void main(uint3 dispatch_thread_id
                                                  : SV_DISPATCHTHREADID) {
    const uint index = dispatch_thread_id.x;

    // @TODO: consider remove this branching
    if (index == 0) {
        GlobalParticleCounter[0].dead_count = MAX_PARTICLE_COUNT;
        GlobalParticleCounter[0].alive_count[0] = 0;
        GlobalParticleCounter[0].alive_count[1] = 0;
    }

    if (index < MAX_PARTICLE_COUNT) {
        // dead particles
        GlobalDeadIndices[index] = index;
    }
}
