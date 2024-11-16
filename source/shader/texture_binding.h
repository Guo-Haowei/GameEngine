/// File: texture_binding.h
#if defined(__cplusplus)
#ifndef SHADER_TEXTURE
#define SHADER_TEXTURE(...)
#endif
#elif defined(HLSL_LANG)
#define SHADER_TEXTURE(TYPE, NAME, SLOT) TYPE NAME : register(t##SLOT);
#else
#define SHADER_TEXTURE(TYPE, NAME, SLOT) uniform TYPE NAME;
#define Texture2D                        sampler2D
#define TextureCube                      samplerCube
#endif

// dynamic srvs
SHADER_TEXTURE(Texture2D, t_baseColorMap, 0)
SHADER_TEXTURE(Texture2D, t_normalMap, 1)
SHADER_TEXTURE(Texture2D, t_materialMap, 2)
SHADER_TEXTURE(Texture2D, t_bloomInputImage, 3)

// static srvs
SHADER_TEXTURE(Texture2D, t_selectionHighlight, 8)
SHADER_TEXTURE(Texture2D, t_gbufferDepth, 9)
SHADER_TEXTURE(Texture2D, t_gbufferBaseColorMap, 10)
SHADER_TEXTURE(Texture2D, t_gbufferPositionMap, 11)
SHADER_TEXTURE(Texture2D, t_gbufferNormalMap, 12)
SHADER_TEXTURE(Texture2D, t_gbufferMaterialMap, 13)
SHADER_TEXTURE(Texture2D, t_textureLighting, 14)

// [SCRUM-34] @TODO: texture cube array
SHADER_TEXTURE(TextureCube, t_pointShadow0, 21)
SHADER_TEXTURE(TextureCube, t_pointShadow1, 22)
SHADER_TEXTURE(TextureCube, t_pointShadow2, 23)
SHADER_TEXTURE(TextureCube, t_pointShadow3, 24)
SHADER_TEXTURE(Texture2D, t_shadowMap, 25)
