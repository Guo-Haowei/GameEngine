#include "../cbuffer.h"
#include "../particle_defines.h"

layout(local_size_x = PARTICLE_LOCAL_SIZE, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint index = gl_GlobalInvocationID.x;

    // @TODO: consider remove this branching
    if (index == 0) {
        Counters.dead_count = MAX_PARTICLE_COUNT;
        Counters.alive_count[0] = 0;
        Counters.alive_count[1] = 0;
    }

    if (index < MAX_PARTICLE_COUNT) {
        // dead particles
        DeadIndices.indices[index] = index;
    }
}
