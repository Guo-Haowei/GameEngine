/// File: texture_binding.hlsl.h
#ifndef TEXTURE_BINDING_HLSL_H_INCLUDED
#define TEXTURE_BINDING_HLSL_H_INCLUDED
#include "shader_defines.hlsl.h"

#define SRV_LIST                                                         \
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
SRV_LIST
#undef SRV
#elif defined(GLSL_LANG)
// @TODO: refactor
#define Texture2D                      sampler2D
#define TextureCube                    samplerCube
#define TextureCubeArray               samplerCubeArray
#define SRV(TYPE, NAME, SLOT, BINDING) uniform TYPE t_##NAME;
SRV_LIST
#undef SRV
#elif defined(__cplusplus)
#define DEFAULT_SHADER_TEXTURE(TYPE, NAME, SLOT, BINDING) \
    static inline constexpr int Get##NAME##Slot() { return SLOT; }
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

#endif
