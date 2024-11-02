// @TODO: refactor LOCAL_SIZE
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

#include "../cbuffer.h"
#include "../particle_defines.h"

void main() {
    const uint PARTICLES_PER_FRAME = 5;

    Counters.emission_count = min(uint(PARTICLES_PER_FRAME), Counters.dead_count);
    Counters.simulation_count = Counters.alive_count[u_PreSimIdx] + Counters.emission_count;
    Counters.alive_count[u_PostSimIdx] = 0;
}
