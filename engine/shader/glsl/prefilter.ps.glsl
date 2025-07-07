/// File: prefilter.ps.glsl
#include "../cbuffer.hlsl.h"
#include "../shader_resource_defines.hlsl.h"
#include "lighting.glsl"

layout(location = 0) out vec4 out_color;
in vec3 out_var_POSITION;

uniform samplerCube u_Texture0;
#define t_Skybox u_Texture0

#include "../shared_prefilter.h"