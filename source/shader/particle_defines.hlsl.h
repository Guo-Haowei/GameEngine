/// File: particle_defines.hlsl.h
#ifndef PARTICLE_DEFINES_INCLUDE
#define PARTICLE_DEFINES_INCLUDE
#include "shader_defines.hlsl.h"

#if defined(__cplusplus)
#define SBUFFER(DATA_TYPE, NAME, REG) \
    constexpr inline int Get##NAME##Slot() { return REG; }
#elif defined(HLSL_LANG)
#define SBUFFER(DATA_TYPE, NAME, REG) \
    RWStructuredBuffer<DATA_TYPE> NAME : register(u##REG);
#elif defined(GLSL_LANG)
#define SBUFFER(DATA_TYPE, NAME, REG) \
    layout(std430, binding = REG) buffer NAME##_t { DATA_TYPE NAME[]; }
#else
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
    int aliveCount[2];
    int deadCount;
    int simulationCount;
    int emissionCount;
};

SBUFFER(ParticleCounter, GlobalParticleCounter, 16);
SBUFFER(int, GlobalDeadIndices, 17);
SBUFFER(int, GlobalAliveIndicesPreSim, 18);
SBUFFER(int, GlobalAliveIndicesPostSim, 19);
SBUFFER(Particle, GlobalParticleData, 20);

#endif
