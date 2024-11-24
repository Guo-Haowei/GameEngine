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

struct gpu_geometry_t {

    Vector3f A;
#if defined(__cplusplus)
    enum class Kind : uint32_t {
        Invalid,
        Triangle,
        Sphere,
        Count
    };

    Kind kind;
#else
    int kind;
#endif

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

#if defined(__cplusplus)
    gpu_geometry_t();
    gpu_geometry_t(const Vector3f& A, const Vector3f& B, const Vector3f& C, int material);
    gpu_geometry_t(const Vector3f& center, float radius, int material);
    Vector3f Centroid() const;
    void CalcNormal();
#endif
};

struct gpu_bvh_t {
    Vector3f min;
    int missIdx;
    Vector3f max;
    int hitIdx;

    int leaf;
    int geomIdx;
    int _padding_0;
    int _padding_1;

#if defined(__cplusplus)
    gpu_bvh_t();
#endif
};

struct gpu_material_t {
    Vector3f albedo;
    float reflectChance;
    Vector3f emissive;
    float roughness;
    float albedoMapLevel;
    int _padding_0;
    int _padding_1;
    int _padding_2;
};

#if defined(__cplusplus)
static_assert(sizeof(gpu_material_t) % sizeof(Vector4f) == 0);
static_assert(sizeof(gpu_bvh_t) % sizeof(Vector4f) == 0);
static_assert(sizeof(gpu_geometry_t) % sizeof(Vector4f) == 0);
#endif

#define SBUFFER_LIST                                         \
    SBUFFER(ParticleCounter, GlobalParticleCounter, 16, 511) \
    SBUFFER(int, GlobalDeadIndices, 17, 510)                 \
    SBUFFER(int, GlobalAliveIndicesPreSim, 18, 509)          \
    SBUFFER(int, GlobalAliveIndicesPostSim, 19, 508)         \
    SBUFFER(Particle, GlobalParticleData, 20, 507)           \
    SBUFFER(gpu_geometry_t, GlobalGeometries, 21, 506)       \
    SBUFFER(gpu_bvh_t, GlobalBvhs, 22, 505)                  \
    SBUFFER(gpu_material_t, GlobalMaterials, 23, 504)

END_NAME_SPACE(my)

#endif
