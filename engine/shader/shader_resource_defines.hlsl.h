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

#define SRV_DEFINES                                 \
    SRV(Texture2D, BaseColorMap, 30, RESOURCE_NONE) \
    SRV(Texture2D, NormalMap, 31, RESOURCE_NONE)    \
    SRV(Texture2D, MaterialMap, 32, RESOURCE_NONE)

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
