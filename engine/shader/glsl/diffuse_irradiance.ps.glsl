/// File: diffuse_irradiance.ps.glsl
#include "../cbuffer.hlsl.h"
#include "../shader_resource_defines.hlsl.h"

layout(location = 0) out vec4 out_color;
in vec3 out_var_POSITION;
#include "../diffuse_irradiance.ps.hlsl.h"
