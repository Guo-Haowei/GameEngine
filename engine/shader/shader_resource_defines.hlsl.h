/// File: shader_resource_defines.hlsl.h
#ifndef TEXTURE_BINDING_HLSL_H_INCLUDED
#define TEXTURE_BINDING_HLSL_H_INCLUDED
#include "structured_buffer.hlsl.h"

#if defined(HLSL_LANG_D3D12)
#include "descriptor_table_defines.hlsl.h"
#define DESCRIPTOR_SRV(TYPE, MAX, PREV, SPACE) TYPE t_##TYPE##s[MAX] : register(t0, space##SPACE);
DESCRIPTOR_SRV_LIST
#undef DESCRIPTOR_SRV
#else  // #if defined(HLSL_LANG_D3D12)

#define SRV_DEFINES                                                                   \
    SRV(Texture2D, BaseColorMap, 0, RESOURCE_NONE)                                    \
    SRV(Texture2D, NormalMap, 1, RESOURCE_NONE)                                       \
    SRV(Texture2D, MaterialMap, 2, RESOURCE_NONE)                                     \
    SRV(Texture2D, BloomInputTexture, 3, RESOURCE_NONE)                               \
    SRV(Texture3D, VoxelLighting, 4, RESOURCE_VOXEL_LIGHTING)                         \
    SRV(Texture3D, VoxelNormal, 5, RESOURCE_VOXEL_NORMAL)                             \
    SRV(Texture2D, ShadowMap, 6, RESOURCE_NONE)                                       \
    SRV(TextureCubeArray, PointShadowArray, 7, RESOURCE_NONE)                         \
    SRV(Texture2D, TextureHighlightSelect, 8, RESOURCE_OUTLINE_SELECT)                \
    SRV(Texture2D, GbufferDepth, 9, RESOURCE_GBUFFER_DEPTH)                           \
    SRV(Texture2D, GbufferBaseColorMap, 10, RESOURCE_GBUFFER_BASE_COLOR)              \
    SRV(Texture2D, GbufferPositionMap, 11, RESOURCE_GBUFFER_POSITION)                 \
    SRV(Texture2D, GbufferNormalMap, 12, RESOURCE_GBUFFER_NORMAL)                     \
    SRV(Texture2D, GbufferMaterialMap, 13, RESOURCE_GBUFFER_MATERIAL)                 \
    SRV(Texture2D, TextureLighting, 14, RESOURCE_LIGHTING)                            \
    SRV(Texture2D, SkyboxHdr, 15, RESOURCE_NONE)                                      \
    SRV(TextureCube, Skybox, 16, RESOURCE_ENV_SKYBOX_CUBE_MAP)                        \
    SRV(Texture2D, BrdfLut, 17, RESOURCE_NONE)                                        \
    SRV(TextureCube, DiffuseIrradiance, 18, RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP) \
    SRV(TextureCube, Prefiltered, 19, RESOURCE_ENV_PREFILTER_CUBE_MAP)

#if defined(HLSL_LANG_D3D11)
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
    [[maybe_unused]] static constexpr inline int Get##NAME##Slot() { return SLOT; }
SRV_DEFINES
#undef SRV
#else
#error Platform not supported
#endif

#endif  // #else // #if defined(HLSL_LANG_D3D12)

// @TODO: refactor
#if defined(HLSL_LANG_D3D12)
#define TEXTURE_2D(NAME)         (t_Texture2Ds[c_##NAME##ResidentHandle.x])
#define TEXTURE_CUBE(NAME)       (t_TextureCubes[c_##NAME##ResidentHandle.x])
#define TEXTURE_CUBE_ARRAY(NAME) (t_TextureCubeArrays[c_##NAME##ResidentHandle.x])
#elif defined(HLSL_LANG_D3D11)
#define TEXTURE_2D(NAME)         (t_##NAME)
#define TEXTURE_CUBE(NAME)       (t_##NAME)
#define TEXTURE_CUBE_ARRAY(NAME) (t_##NAME)
#endif

#if defined(HLSL_LANG_D3D11)
#define SBUFFER(DATA_TYPE, NAME, REG, REG2) \
    RWStructuredBuffer<DATA_TYPE> NAME : register(u##REG);
#endif

#if defined(HLSL_LANG_D3D12)
#define SBUFFER(DATA_TYPE, NAME, REG, REG2) \
    RWStructuredBuffer<DATA_TYPE> NAME : register(u##REG2, space##REG);
#endif

#if defined(GLSL_LANG)
#define SBUFFER(DATA_TYPE, NAME, REG, REG2) \
    layout(std430, binding = REG) buffer NAME##_t { DATA_TYPE NAME[]; };
#endif

#if defined(__cplusplus)
#define SBUFFER(DATA_TYPE, NAME, REG, REG2) \
    static constexpr inline int Get##NAME##Slot() { return REG; }
#endif

SBUFFER_LIST

#undef SBUFFER

#endif
