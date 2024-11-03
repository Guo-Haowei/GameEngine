#include "cbuffer.h"
#include "particle_defines.h"

[numthreads(1, 1, 1)] void main(uint3 dispatch_thread_id
                                : SV_DISPATCHTHREADID) {
    GlobalParticleCounter[0].emission_count = min(uint(u_ParticlesPerFrame), GlobalParticleCounter[0].dead_count);
    GlobalParticleCounter[0].simulation_count = GlobalParticleCounter[0].alive_count[u_PreSimIdx] + GlobalParticleCounter[0].emission_count;
    GlobalParticleCounter[0].alive_count[u_PostSimIdx] = 0;
}
