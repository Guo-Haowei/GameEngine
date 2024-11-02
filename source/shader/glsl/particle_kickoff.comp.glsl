// @TODO: refactor LOCAL_SIZE
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

#include "../cbuffer.h"
#include "../particle_defines.h"

void main() {
    Counters.emission_count = min(uint(u_ParticlesPerFrame), Counters.dead_count);
    Counters.simulation_count = Counters.alive_count[u_PreSimIdx] + Counters.emission_count;
    Counters.alive_count[u_PostSimIdx] = 0;
}
