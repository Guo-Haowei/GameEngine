#ifndef PARTICLE_DEFINES_INCLUDE
#define PARTICLE_DEFINES_INCLUDE

#if defined(HLSL_LANG)
#error SBUFFER NOT DEFINED
#else
#define SBUFFER(NAME, REG) layout(std430, binding = REG) buffer NAME
#endif

// @TODO: shader naming style
struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 color;

    float scale;
    float lifeSpan;
    float lifeRemaining;
    int isActive;
};

layout(std430, binding = 0) buffer ParticleData_t {
    Particle particles[];
}
ParticleData;

layout(std430, binding = 1) buffer ParticleCounters_t {
    uint dead_count;
    uint alive_count[2];
    uint simulation_count;
    uint emission_count;
}
Counters;

layout(std430, binding = 2) buffer ParticleDeadIndices_t {
    uint indices[];
}
DeadIndices;

layout(std430, binding = 3) buffer ParticleAlivePreSimIndices_t {
    uint indices[];
}
AliveIndicesPreSim;

layout(std430, binding = 4) buffer ParticleAlivePostSimIndices_t {
    uint indices[];
}
AliveIndicesPostSim;

#endif
