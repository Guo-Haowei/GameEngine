/// File: skybox.ps.glsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

float4 main(vsoutput_position input) : SV_TARGET {
    float3 uvw = normalize(input.world_position);
#if !defined(HLSL_2_GLSL)
    uvw.y = -uvw.y;
#endif
    float3 color = TEXTURE_CUBE(Skybox).Sample(s_cubemapClampSampler, uvw).rgb;
    // color = color / (color + float3(1.0, 1.0, 1.0));
    // color = pow(color, float3(1.0, 1.0, 1.0) / 2.2);
    return float4(color, 1.0f);
}
