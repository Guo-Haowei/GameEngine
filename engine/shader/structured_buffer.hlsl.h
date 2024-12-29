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

// ray tracing
struct GpuPtBvh {
    Vector3f min;
    int missIdx;
    Vector3f max;
    int hitIdx;

    Vector2f _padding;
    int leaf;
    int triangleIndex;
};

struct GpuPtVertex {
    Vector3f position;
    int _padding1;
    Vector3f normal;
    int _padding2;
};

struct GpuPtIndex {
    Vector3i tri;
    int _padding1;
};

struct GpuPtMesh {
    Matrix4x4f transform;
    Matrix4x4f transformInv;

    Vector2f _padding4;
    int rootBvhId;
    int materialId;
};

struct GpuPtMaterial {
    Vector3f baseColor;
    float roughness;
    Vector3f emissive;
    float metallic;
};

#ifdef __cplusplus
static_assert(sizeof(GpuPtBvh) % sizeof(Vector4f) == 0);
static_assert(sizeof(GpuPtVertex) % sizeof(Vector4f) == 0);
static_assert(sizeof(GpuPtIndex) % sizeof(Vector4f) == 0);
static_assert(sizeof(GpuPtMesh) % sizeof(Vector4f) == 0);
static_assert(sizeof(GpuPtMaterial) % sizeof(Vector4f) == 0);
#endif  // __cplusplus

#define SBUFFER_LIST                                         \
    SBUFFER(ParticleCounter, GlobalParticleCounter, 16, 511) \
    SBUFFER(int, GlobalDeadIndices, 17, 510)                 \
    SBUFFER(int, GlobalAliveIndicesPreSim, 18, 509)          \
    SBUFFER(int, GlobalAliveIndicesPostSim, 19, 508)         \
    SBUFFER(Particle, GlobalParticleData, 20, 507)           \
    SBUFFER(GpuPtVertex, GlobalPtVertices, 21, 506)          \
    SBUFFER(GpuPtIndex, GlobalPtIndices, 22, 505)            \
    SBUFFER(GpuPtBvh, GlobalPtBvhs, 23, 504)                 \
    SBUFFER(GpuPtMesh, GlobalPtMeshes, 24, 503)              \
    SBUFFER(GpuPtMaterial, GlobalPtMaterials, 25, 502)

#endif
