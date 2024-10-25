#if defined(__cplusplus)
#ifndef SHADER_TEXTURE
#define SHADER_TEXTURE(...)
#endif
#elif defined(HLSL_LANG)
#define SHADER_TEXTURE(TYPE, NAME, SLOT) TYPE NAME : register(t##SLOT);
#else
#define SHADER_TEXTURE(TYPE, NAME, SLOT) uniform TYPE NAME;
#define Texture2D                        sampler2D
#endif

SHADER_TEXTURE(Texture2D, u_base_color_map, 0)
SHADER_TEXTURE(Texture2D, u_normal_map, 1)
SHADER_TEXTURE(Texture2D, u_material_map, 2)

SHADER_TEXTURE(Texture2D, u_shadow_map, 8)

SHADER_TEXTURE(Texture2D, u_selection_highlight, 9)
SHADER_TEXTURE(Texture2D, u_gbuffer_base_color_map, 10)
SHADER_TEXTURE(Texture2D, u_gbuffer_position_map, 11)
SHADER_TEXTURE(Texture2D, u_gbuffer_normal_map, 12)
SHADER_TEXTURE(Texture2D, u_gbuffer_material_map, 13)
SHADER_TEXTURE(Texture2D, g_texture_lighting, 14)
SHADER_TEXTURE(Texture2D, g_bloom_input_image, 15)
