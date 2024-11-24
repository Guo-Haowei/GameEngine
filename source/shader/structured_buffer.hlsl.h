#ifndef STRUCTURED_BUFFER_HLSL_H_INCLUDED
#define STRUCTURED_BUFFER_HLSL_H_INCLUDED
#include "shader_defines.hlsl.h"

BEGIN_NAME_SPACE(my)

struct Particle {
    Vector4f position;
    Vector4f velocity;
    Vector4f color;

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

#define SBUFFER_LIST                                         \
    SBUFFER(ParticleCounter, GlobalParticleCounter, 16, 511) \
    SBUFFER(int, GlobalDeadIndices, 17, 510)                 \
    SBUFFER(int, GlobalAliveIndicesPreSim, 18, 509)          \
    SBUFFER(int, GlobalAliveIndicesPostSim, 19, 508)         \
    SBUFFER(Particle, GlobalParticleData, 24, 507)

END_NAME_SPACE(my)

#endif
