/// File: particle_emission.cs.hlsl
#include "cbuffer.hlsl.h"
#include "shader_resource_defines.hlsl.h"

float Random(float3 co, uint index) {
    return frac(sin(dot(float4(co.xyz, float(index)), float4(12.9898, 78.233, 45.5432, 22.1458))) * 43758.5453);
}

uint pop_dead_index() {
    int index;
    InterlockedAdd(GlobalParticleCounter[0].deadCount, -1, index);
    return GlobalDeadIndices[index - 1];
}

void push_alive_index(uint p_index) {
    uint insert_index;
    InterlockedAdd(GlobalParticleCounter[0].aliveCount[c_preSimIdx], 1, insert_index);
    GlobalAliveIndicesPreSim[insert_index] = p_index;
}

[numthreads(PARTICLE_LOCAL_SIZE, 1, 1)] void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    uint index = dispatch_thread_id.x;

    if (index < GlobalParticleCounter[0].emissionCount) {
        uint particle_index = pop_dead_index();

        float3 velocity = c_emitterStartingVelocity;
        velocity.x += Random(c_seeds.xyz, (index + 1)) - 0.5;
        velocity.y += Random(c_seeds.yzx, (index + 1)) - 0.5;
        velocity.z += Random(c_seeds.zxy, (index + 1)) - 0.5;

        float3 color;
        color.r = Random(c_seeds.xzy, index);
        color.g = Random(c_seeds.zyx, index);
        color.b = Random(c_seeds.yzx, index);
        color *= 3.0;

        GlobalParticleData[particle_index].position.xyz = c_emitterPosition;
        GlobalParticleData[particle_index].velocity.xyz = velocity;
        GlobalParticleData[particle_index].lifeSpan = c_lifeSpan;
        GlobalParticleData[particle_index].lifeRemaining = c_lifeSpan;
        GlobalParticleData[particle_index].scale = c_emitterScale;
        GlobalParticleData[particle_index].color.rgb = color;

        push_alive_index(particle_index);
    }
}