#include "cbuffer.h"
#include "particle_defines.h"

float Random(float3 co) {
    return frac(sin(dot(co.xyz, float3(12.9898, 78.233, 45.5432))) * 43758.5453);
}

uint pop_dead_index() {
    int index;
    InterlockedAdd(GlobalParticleCounter[0].dead_count, -1, index);
    return GlobalDeadIndices[index - 1];
}

void push_alive_index(uint p_index) {
    uint insert_index;
    InterlockedAdd(GlobalParticleCounter[0].alive_count[u_PreSimIdx], 1, insert_index);
    GlobalAliveIndicesPreSim[insert_index] = p_index;
}

[numthreads(PARTICLE_LOCAL_SIZE, 1, 1)] void main(uint3 dispatch_thread_id
                                                  : SV_DISPATCHTHREADID) {
    uint index = dispatch_thread_id.x;

    if (index < GlobalParticleCounter[0].emission_count) {
        uint particle_index = pop_dead_index();

        float3 velocity = u_Velocity;
        velocity.x += Random(u_Seeds.xyz / (index + 1)) - 0.5;
        velocity.y += Random(u_Seeds.yzx / (index + 1)) - 0.5;
        velocity.z += Random(u_Seeds.zxy / (index + 1)) - 0.5;

        float3 color;
        color.r = Random(u_Seeds.xzy / (index + 1));
        color.g = Random(u_Seeds.zyx / (index + 1));
        color.b = Random(u_Seeds.yzx / (index + 1));
        color = 2.0 * color;

        GlobalParticleData[particle_index].position.xyz = u_Position;
        GlobalParticleData[particle_index].velocity.xyz = velocity;
        GlobalParticleData[particle_index].lifeSpan = u_LifeSpan;
        GlobalParticleData[particle_index].lifeRemaining = u_LifeSpan;
        GlobalParticleData[particle_index].scale = u_Scale;
        GlobalParticleData[particle_index].color.rgb = color;

        push_alive_index(particle_index);
    }
}