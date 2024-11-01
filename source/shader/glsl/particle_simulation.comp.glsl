layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "../cbuffer.h"

layout(std430, binding = 0) buffer ParticleData {
    vec4 globalParticles[];
};

void main() {
    globalParticles[gl_GlobalInvocationID.x] = globalParticleTransforms[gl_GlobalInvocationID.x];
}
