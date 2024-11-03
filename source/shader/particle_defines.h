#ifndef PARTICLE_DEFINES_INCLUDE
#define PARTICLE_DEFINES_INCLUDE
#include "shader_defines.h"

#if defined(HLSL_LANG)
#define SBUFFER(DATA_TYPE, NAME, REG) \
    RWStructuredBuffer<DATA_TYPE> NAME : register(u##REG);
#else
#define SBUFFER(DATA_TYPE, NAME, REG) \
    layout(std430, binding = REG) buffer NAME##_t { DATA_TYPE NAME[]; }
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

struct ParticleCounter {
    uint alive_count[2];
    uint dead_count;
    uint simulation_count;
    uint emission_count;
};

SBUFFER(ParticleCounter, GlobalParticleCounter, 16);
SBUFFER(uint, GlobalDeadIndices, 17);
SBUFFER(uint, GlobalAliveIndicesPreSim, 18);
SBUFFER(uint, GlobalAliveIndicesPostSim, 19);
SBUFFER(Particle, GlobalParticleData, 20);

#endif
