#if defined(__cplusplus)
#ifndef SHADER_TEXTURE
#define SHADER_TEXTURE(...)
#endif
#elif defined(HLSL_LANG)
#else
#define SHADER_TEXTURE(TYPE, NAME, SLOT) uniform TYPE NAME;
#endif

SHADER_TEXTURE(sampler2D, u_gbuffer_base_color_map, 10)
SHADER_TEXTURE(sampler2D, u_gbuffer_position_map, 11)
SHADER_TEXTURE(sampler2D, u_gbuffer_normal_map, 12)
SHADER_TEXTURE(sampler2D, u_gbuffer_material_map, 13)
