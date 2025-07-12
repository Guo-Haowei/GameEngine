/// File: skybox.ps.glsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

TextureCube t_Skybox : register(t0);

#if 0
float2 sample_spherical_map(float3 v) {
    const float2 inv_atan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}
#endif

float4 main(VS_OUTPUT_POSITION input)
    : SV_TARGET {
    float3 uvw = input.world_position;
#if !defined(HLSL_2_GLSL)
    uvw.y = -uvw.y;
#endif
    float3 color = t_Skybox.SampleLevel(s_cubemapClampSampler, uvw, 0.0f).rgb;
    //  color = color / (color + float3(1.0, 1.0, 1.0));
    //  color = pow(color, float3(1.0, 1.0, 1.0) / 2.2);
    return float4(color, 1.0f);
}
