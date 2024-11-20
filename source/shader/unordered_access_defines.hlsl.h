#ifndef UNORDERED_ACCESS_DEFINES_HLSL_H_INCLUDED
#define UNORDERED_ACCESS_DEFINES_HLSL_H_INCLUDED

#define UAV_DEFINES \
    UAV(RWTexture2D<float3>, BloomOutputImage, 3)

#if defined(__cplusplus)
#define UAV(TYPE, NAME, SLOT) \
    [[maybe_unused]] static inline constexpr int GetUavSlot##NAME() { return SLOT; }
UAV_DEFINES
#undef UAV
#elif defined(HLSL_LANG)
#define UAV(TYPE, NAME, SLOT) TYPE u_##NAME : register(u##SLOT);
UAV_DEFINES
#undef UAV
#elif defined(GLSL_LANG)
#else
#error Shading language not supported
#endif

#endif
