#ifndef STRUCTURED_BUFFER_HLSL_H_INCLUDED
#define STRUCTURED_BUFFER_HLSL_H_INCLUDED
#include "cbuffer.hlsl.h"

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

#if defined(__cplusplus)
#define VEC3 NewVector3f
#else
#define VEC3 Vector3f
#endif

struct gpu_geometry_t {

    VEC3 A;
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

    VEC3 B;
    float radius;

    VEC3 C;
    int material_id;

    Vector2f uv1;
    float _padding_0;
    float _padding_1;

    Vector2f uv2;
    float _padding_2;
    float _padding_3;

    Vector2f uv3;
    float _padding_4;
    float _padding_5;

    VEC3 normal1;
    float _padding_6;

    VEC3 normal2;
    float _padding_7;

    VEC3 normal3;
    float _padding_8;

#if defined(__cplusplus)
    gpu_geometry_t();
    gpu_geometry_t(const VEC3& A, const VEC3& B, const VEC3& C, int material);
    gpu_geometry_t(const VEC3& center, float radius, int material);
    VEC3 Centroid() const;
    void CalcNormal();
#endif
};

struct gpu_bvh_t {
    VEC3 min;
    int missIdx;
    VEC3 max;
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
    float reflect_chance;

    Vector3f emissive;
    float roughness;

    int has_base_color_map;
    int has_normal_map;
    int has_material_map;
    int has_height_map;

    sampler2D base_color_map_handle;
    sampler2D normal_map_handle;
    sampler2D material_map_handle;
    sampler2D height_map_handle;
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
