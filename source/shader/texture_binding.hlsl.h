/// File: texture_binding.hlsl.h
#if defined(__cplusplus)
#ifndef SHADER_TEXTURE
#define SHADER_TEXTURE(...)
#endif
#elif defined(HLSL_LANG)
#define SHADER_TEXTURE(TYPE, NAME, SLOT, BINDING) TYPE NAME : register(t##SLOT);
#else
#define SHADER_TEXTURE(TYPE, NAME, SLOT, BINDING) uniform TYPE NAME;
#define Texture2D                                 sampler2D
#define TextureCube                               samplerCube
#define TextureCubeArray                          samplerCubeArray
#endif

// dynamic srvs
SHADER_TEXTURE(Texture2D, t_baseColorMap, 0, RESOURCE_NONE)
SHADER_TEXTURE(Texture2D, t_normalMap, 1, RESOURCE_NONE)
SHADER_TEXTURE(Texture2D, t_materialMap, 2, RESOURCE_NONE)
SHADER_TEXTURE(Texture2D, t_bloomInputImage, 3, RESOURCE_NONE)

// static srvs
SHADER_TEXTURE(Texture2D, t_selectionHighlight, 8, RESOURCE_HIGHLIGHT_SELECT)
SHADER_TEXTURE(Texture2D, t_gbufferDepth, 9, RESOURCE_GBUFFER_DEPTH)
SHADER_TEXTURE(Texture2D, t_gbufferBaseColorMap, 10, RESOURCE_GBUFFER_BASE_COLOR)
SHADER_TEXTURE(Texture2D, t_gbufferPositionMap, 11, RESOURCE_GBUFFER_POSITION)
SHADER_TEXTURE(Texture2D, t_gbufferNormalMap, 12, RESOURCE_GBUFFER_NORMAL)
SHADER_TEXTURE(Texture2D, t_gbufferMaterialMap, 13, RESOURCE_GBUFFER_MATERIAL)
SHADER_TEXTURE(Texture2D, t_textureLighting, 14, RESOURCE_LIGHTING)

// [SCRUM-34] @TODO: texture cube array
SHADER_TEXTURE(TextureCube, t_pointShadowArray, 21, RESOURCE_NONE)
SHADER_TEXTURE(Texture2D, t_shadowMap, 25, RESOURCE_NONE)
