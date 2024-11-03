#include "cbuffer.h"
#include "particle_defines.h"

void push_dead_index(uint index) {
    uint insert_idx;
    InterlockedAdd(GlobalParticleCounter[0].dead_count, 1, insert_idx);
    GlobalDeadIndices[insert_idx] = index;
}

uint pop_dead_index() {
    uint index;
    InterlockedAdd(GlobalParticleCounter[0].dead_count, -1, index);
    return GlobalDeadIndices[index - 1];
}

void push_alive_index(uint index) {
    uint insert_idx;
    InterlockedAdd(GlobalParticleCounter[0].alive_count[u_PostSimIdx], 1, insert_idx);
    GlobalAliveIndicesPostSim[insert_idx] = index;
}

uint pop_alive_index() {
    uint index;
    InterlockedAdd(GlobalParticleCounter[0].alive_count[u_PreSimIdx], -1, index);
    return GlobalAliveIndicesPreSim[index - 1];
}

[numthreads(PARTICLE_LOCAL_SIZE, 1, 1)] void main(uint3 dispatch_thread_id
                                                  : SV_DISPATCHTHREADID) {
    uint index = dispatch_thread_id.x;

    if (index < GlobalParticleCounter[0].simulation_count) {
        // Consume an Alive particle index
        uint particle_index = pop_alive_index();

        Particle particle = GlobalParticleData[particle_index];

        // Is it dead?
        if (particle.lifeRemaining <= 0.0f) {
            push_dead_index(particle_index);
        } else {
            particle.lifeRemaining.x -= u_ElapsedTime;
            particle.position += u_ElapsedTime * particle.velocity;

            GlobalParticleData[particle_index] = particle;

            push_alive_index(particle_index);
        }
    }
}
