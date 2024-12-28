#ifndef STRUCTURED_BUFFER_HLSL_H_INCLUDED
#define STRUCTURED_BUFFER_HLSL_H_INCLUDED
#include "cbuffer.hlsl.h"

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

// Ray tracinge
struct GpuBvhAccel {
    Vector3f min;
    int missIdx;
    Vector3f max;
    int hitIdx;

    int leaf;
    int meshIndex;
    int triangleIndex;
    int _padding_1;
};

struct GpuTriangleVertex {
    Vector3f vertex;
};

struct GpuTriangleIndex {
    Vector3i indices;
};

#define SBUFFER_LIST                                            \
    SBUFFER(ParticleCounter, GlobalParticleCounter, 16, 511)    \
    SBUFFER(int, GlobalDeadIndices, 17, 510)                    \
    SBUFFER(int, GlobalAliveIndicesPreSim, 18, 509)             \
    SBUFFER(int, GlobalAliveIndicesPostSim, 19, 508)            \
    SBUFFER(Particle, GlobalParticleData, 20, 507)              \
    SBUFFER(GpuTriangleVertex, GlobalTriangleVertices, 21, 506) \
    SBUFFER(GpuTriangleIndex, GlobalTriangleIndices, 22, 505)   \
    SBUFFER(GpuBvhAccel, GlobalBvhs, 23, 504)

#endif
