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

struct Geometry {
    Vector3f A;
    int kind;

    Vector3f B;
    float radius;

    Vector3f C;
    int materialId;

    Vector2f uv1;
    Vector2f uv2;

    Vector3f normal1;
    float uv3x;
    Vector3f normal2;
    float uv3y;

    Vector3f normal3;
    float hasAlbedoMap;
};

struct Bvh {
    Vector3f min;
    int missIdx;
    Vector3f max;
    int hitIdx;

    int leaf;
    int geomIdx;
    int _padding0;
    int _padding1;
};

struct Material {
    Vector3f albedo;
    float reflectChance;
    Vector3f emissive;
    float roughness;
    float albedoMapLevel;
    int _padding0;
    int _padding1;
    int _padding2;
};

#if defined(__cplusplus)
static_assert(sizeof(Material) % 4 == 0);
static_assert(sizeof(Bvh) % 4 == 0);
static_assert(sizeof(Geometry) % 4 == 0);
#endif

#define SBUFFER_LIST                                         \
    SBUFFER(ParticleCounter, GlobalParticleCounter, 16, 511) \
    SBUFFER(int, GlobalDeadIndices, 17, 510)                 \
    SBUFFER(int, GlobalAliveIndicesPreSim, 18, 509)          \
    SBUFFER(int, GlobalAliveIndicesPostSim, 19, 508)         \
    SBUFFER(Particle, GlobalParticleData, 20, 507)           \
    SBUFFER(Geometry, GlobalGeometries, 21, 506)             \
    SBUFFER(Bvh, GlobalBvhs, 22, 505)

END_NAME_SPACE(my)

#endif
