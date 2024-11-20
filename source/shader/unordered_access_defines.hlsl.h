#ifndef UNORDERED_ACCESS_DEFINES_HLSL_H_INCLUDED
#define UNORDERED_ACCESS_DEFINES_HLSL_H_INCLUDED

#define UAV_DEFINES \
    UAV(RWTexture2D<float4>, BloomOutputImage, 3)

#if defined(__cplusplus)
#define UAV(TYPE, NAME, SLOT) \
    [[maybe_unused]] static inline constexpr int GetUavSlot##NAME() { return SLOT; }
UAV_DEFINES
#undef UAV
#elif defined(HLSL_LANG_D3D11)
#define UAV(TYPE, NAME, SLOT) TYPE u_##NAME : register(u##SLOT);
UAV_DEFINES
#undef UAV
#elif defined(HLSL_LANG_D3D12)
RWTexture2D<float3> u_RWTexture2D[8] : register(u0, space2);
#elif defined(GLSL_LANG)
#else
#error Shading language not supported
#endif

#if defined(HLSL_LANG_D3D11)
#define IMAGE_2D(a) (u_##a)
#else
#define IMAGE_2D(a) (u_RWTexture2D[c_##a##Index])
#endif

#endif
