layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "../cbuffer.h"
#include "../particle_defines.h"

// https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/
uint WangHash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

// random number between 0 and 1
float Random(inout uint state) {
    return float(WangHash(state)) / 4294967296.0;
}

uint pop_dead_index() {
    uint index = atomicAdd(Counters.dead_count, -1);
    return DeadIndices.indices[index - 1];
}

void push_alive_index(uint index) {
    uint insert_idx = atomicAdd(Counters.alive_count[u_PreSimIdx], 1);
    AliveIndicesPreSim.indices[insert_idx] = index;
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint z = gl_GlobalInvocationID.z;

    if (x < Counters.emission_count) {
        uint particle_index = pop_dead_index();

        uint seed = uint(uint(x) * uint(1973) + uint(y) * uint(9277) + uint(z) * uint(26699)) | uint(1);

        vec3 position = vec3(0.0, 1.0, 0.0);
        vec3 initial_velocity = vec3(0.0, 1.0, 0.0);
        vec3 velocity = initial_velocity;
        velocity.x += Random(seed);
        velocity.y += Random(seed);
        velocity.z += Random(seed);

        ParticleData.particles[particle_index].position.xyz = position;
        ParticleData.particles[particle_index].velocity.xyz = velocity;
        ParticleData.particles[particle_index].lifeSpan = u_LifeSpan;
        ParticleData.particles[particle_index].lifeRemaining = u_LifeSpan;
        ParticleData.particles[particle_index].scale = 0.02;

        push_alive_index(particle_index);
    }
}