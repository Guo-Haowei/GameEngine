/// File: texture_binding.hlsl.h
#ifndef TEXTURE_BINDING_HLSL_H_INCLUDED
#define TEXTURE_BINDING_HLSL_H_INCLUDED

#if defined(__cplusplus)
#elif defined(HLSL_LANG)
#define SHADER_TEXTURE(TYPE, NAME, SLOT, BINDING) TYPE t_##NAME : register(t##SLOT);
#else
#define SHADER_TEXTURE(TYPE, NAME, SLOT, BINDING) uniform TYPE t_##NAME;
#define Texture2D                                 sampler2D
#define TextureCube                               samplerCube
#define TextureCubeArray                          samplerCubeArray
#endif

#define SHADER_TEXTURE_LIST                                                         \
    SHADER_TEXTURE(Texture2D, BaseColorMap, 0, RESOURCE_NONE)                       \
    SHADER_TEXTURE(Texture2D, NormalMap, 1, RESOURCE_NONE)                          \
    SHADER_TEXTURE(Texture2D, MaterialMap, 2, RESOURCE_NONE)                        \
    SHADER_TEXTURE(Texture2D, BloomInputImage, 3, RESOURCE_NONE)                    \
    SHADER_TEXTURE(Texture2D, ShadowMap, 6, RESOURCE_NONE)                          \
    SHADER_TEXTURE(TextureCubeArray, PointShadowArray, 7, RESOURCE_NONE)            \
    SHADER_TEXTURE(Texture2D, TextureHighlightSelect, 8, RESOURCE_HIGHLIGHT_SELECT) \
    SHADER_TEXTURE(Texture2D, GbufferDepth, 9, RESOURCE_GBUFFER_DEPTH)              \
    SHADER_TEXTURE(Texture2D, GbufferBaseColorMap, 10, RESOURCE_GBUFFER_BASE_COLOR) \
    SHADER_TEXTURE(Texture2D, GbufferPositionMap, 11, RESOURCE_GBUFFER_POSITION)    \
    SHADER_TEXTURE(Texture2D, GbufferNormalMap, 12, RESOURCE_GBUFFER_NORMAL)        \
    SHADER_TEXTURE(Texture2D, GbufferMaterialMap, 13, RESOURCE_GBUFFER_MATERIAL)    \
    SHADER_TEXTURE(Texture2D, TextureLighting, 14, RESOURCE_LIGHTING)

#if defined(HLSL_LANG_D3D12)
Texture2D t_texture2Ds[MAX_TEXTURE_2D_COUNT] : register(t0, space0);
TextureCubeArray t_textureCubeArrays[MAX_TEXTURE_CUBE_ARRAY_COUNT] : register(t0, space1);
#elif defined(HLSL_LANG_D3D11) || defined(GLSL_LANG)
SHADER_TEXTURE_LIST
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
