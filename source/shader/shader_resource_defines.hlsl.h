/// File: shader_resource_defines.hlsl.h
#ifndef TEXTURE_BINDING_HLSL_H_INCLUDED
#define TEXTURE_BINDING_HLSL_H_INCLUDED
#include "shader_defines.hlsl.h"

#define SRV_DEFINES                                                      \
    SRV(Texture2D, BaseColorMap, 0, RESOURCE_NONE)                       \
    SRV(Texture2D, NormalMap, 1, RESOURCE_NONE)                          \
    SRV(Texture2D, MaterialMap, 2, RESOURCE_NONE)                        \
    SRV(Texture2D, BloomInputImage, 3, RESOURCE_NONE)                    \
    SRV(Texture2D, ShadowMap, 6, RESOURCE_NONE)                          \
    SRV(TextureCubeArray, PointShadowArray, 7, RESOURCE_NONE)            \
    SRV(Texture2D, TextureHighlightSelect, 8, RESOURCE_HIGHLIGHT_SELECT) \
    SRV(Texture2D, GbufferDepth, 9, RESOURCE_GBUFFER_DEPTH)              \
    SRV(Texture2D, GbufferBaseColorMap, 10, RESOURCE_GBUFFER_BASE_COLOR) \
    SRV(Texture2D, GbufferPositionMap, 11, RESOURCE_GBUFFER_POSITION)    \
    SRV(Texture2D, GbufferNormalMap, 12, RESOURCE_GBUFFER_NORMAL)        \
    SRV(Texture2D, GbufferMaterialMap, 13, RESOURCE_GBUFFER_MATERIAL)    \
    SRV(Texture2D, TextureLighting, 14, RESOURCE_LIGHTING)

#if defined(HLSL_LANG_D3D12)
Texture2D t_texture2Ds[MAX_TEXTURE_2D_COUNT] : register(t0, space0);
TextureCubeArray t_textureCubeArrays[MAX_TEXTURE_CUBE_ARRAY_COUNT] : register(t0, space1);
#elif defined(HLSL_LANG_D3D11)
#define SRV(TYPE, NAME, SLOT, BINDING) TYPE t_##NAME : register(t##SLOT);
SRV_DEFINES
#undef SRV
#elif defined(GLSL_LANG)
// @TODO: refactor
#define Texture2D                      sampler2D
#define TextureCube                    samplerCube
#define TextureCubeArray               samplerCubeArray
#define SRV(TYPE, NAME, SLOT, BINDING) uniform TYPE t_##NAME;
SRV_DEFINES
#undef SRV
#elif defined(__cplusplus)
#define SRV(TYPE, NAME, SLOT, BINDING) \
    [[maybe_unused]] static inline constexpr int Get##NAME##Slot() { return SLOT; }
SRV_DEFINES
#undef SRV
#else
#error Platform not supported
#endif

// @TODO: refactor
#if defined(HLSL_LANG_D3D12)
#define TEXTURE_2D(NAME)         (t_texture2Ds[c_##NAME##Index])
#define TEXTURE_CUBE_ARRAY(NAME) (t_textureCubeArrays[c_##NAME##Index])
#elif defined(HLSL_LANG_D3D11)
#define TEXTURE_2D(NAME)         (t_##NAME)
#define TEXTURE_CUBE_ARRAY(NAME) (t_##NAME)
#endif

#if defined(__cplusplus)
#define SBUFFER(DATA_TYPE, NAME, REG) \
    static constexpr inline int Get##NAME##Slot() { return REG; }
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

SBUFFER(ParticleCounter, GlobalParticleCounter, 16);
SBUFFER(int, GlobalDeadIndices, 17);
SBUFFER(int, GlobalAliveIndicesPreSim, 18);
SBUFFER(int, GlobalAliveIndicesPostSim, 19);
SBUFFER(Particle, GlobalParticleData, 24);

#endif
