layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#include "../cbuffer.h"
#include "../particle_defines.h"

float Random(vec3 co) {
    return fract(sin(dot(co.xyz, vec3(12.9898, 78.233, 45.5432))) * 43758.5453);
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
    uint index = gl_GlobalInvocationID.x;

    if (index < Counters.emission_count) {
        uint particle_index = pop_dead_index();

        vec3 velocity = u_Velocity;
        velocity.x += Random(u_Seeds.xyz / (index + 1)) - 0.5;
        velocity.y += Random(u_Seeds.yzx / (index + 1)) - 0.5;
        velocity.z += Random(u_Seeds.zxy / (index + 1)) - 0.5;

        ParticleData.particles[particle_index].position.xyz = u_Position;
        ParticleData.particles[particle_index].velocity.xyz = velocity;
        ParticleData.particles[particle_index].lifeSpan = u_LifeSpan;
        ParticleData.particles[particle_index].lifeRemaining = u_LifeSpan;
        ParticleData.particles[particle_index].scale = 0.02;

        push_alive_index(particle_index);
    }
}