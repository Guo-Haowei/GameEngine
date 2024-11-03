#include "cbuffer.h"
#include "particle_defines.h"

[numthreads(1, 1, 1)] void main(uint3 dispatch_thread_id
                                : SV_DISPATCHTHREADID) {
    int max_allocs = max(u_MaxParticleCount - int(GlobalParticleCounter[0].alive_count[u_PreSimIdx]), 0);

    uint emission_count = min(uint(u_ParticlesPerFrame), GlobalParticleCounter[0].dead_count);
    emission_count = min(uint(max_allocs), emission_count);

    GlobalParticleCounter[0].emission_count = emission_count;
    GlobalParticleCounter[0].simulation_count = GlobalParticleCounter[0].alive_count[u_PreSimIdx] + GlobalParticleCounter[0].emission_count;
    GlobalParticleCounter[0].alive_count[u_PostSimIdx] = 0;
}
