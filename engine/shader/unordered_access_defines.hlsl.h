#ifndef UNORDERED_ACCESS_DEFINES_HLSL_H_INCLUDED
#define UNORDERED_ACCESS_DEFINES_HLSL_H_INCLUDED

#define UAV_DEFINES                                    \
    UAV(RWTexture3D<float4>, VoxelLighting, 0)         \
    UAV(RWTexture3D<float4>, VoxelNormal, 1)           \
    UAV(RWTexture2D<float4>, PathTracerOutputImage, 2) \
    UAV(RWTexture2D<float3>, BloomOutputImage, 3)

#if defined(__cplusplus)
#define UAV(TYPE, NAME, SLOT) \
    [[maybe_unused]] static constexpr inline int GetUavSlot##NAME() { return SLOT; }
UAV_DEFINES
#undef UAV
#elif defined(HLSL_LANG_D3D11)
#define UAV(TYPE, NAME, SLOT) TYPE u_##NAME : register(u##SLOT);
UAV_DEFINES
#undef UAV
#elif defined(HLSL_LANG_D3D12)
#include "descriptor_table_defines.hlsl.h"
#define DESCRIPTOR_UAV(TYPE, MAX, PREV, SPACE, DATA_TYPE) \
    TYPE<DATA_TYPE> u_##TYPE##s[MAX] : register(u0, space##SPACE);
DESCRIPTOR_UAV_LIST
#undef DESCRIPTOR_UAV
// RWTexture2D<float3> u_RWTexture2D[8] : register(u0, space2);
#elif defined(GLSL_LANG)
#else
#error Shading language not supported
#endif

#if defined(HLSL_LANG_D3D11)
#define RW_TEXTURE_2D(a) (u_##a)
#else
#define RW_TEXTURE_2D(a) (u_RWTexture2Ds[c_##a##ResidentHandle.x])
#endif

#endif
