layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "../cbuffer.h"
#include "../particle_defines.h"

void main() {
    uint index = gl_GlobalInvocationID.x;
    globalParticles[index].position = globalParticleTransforms[index].xyz;
    globalParticles[index].scale = globalParticleTransforms[index].w;
}
