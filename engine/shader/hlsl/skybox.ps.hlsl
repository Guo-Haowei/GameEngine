/// File: skybox.ps.glsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

float2 sample_spherical_map(float3 v) {
    const float2 inv_atan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

float4 main(vsoutput_position input) : SV_TARGET {
    float2 uv = sample_spherical_map(input.world_position);
    float3 color = TEXTURE_2D(SkyboxHdr).Sample(s_cubemapClampSampler, uv).rgb;
    // color = color / (color + float3(1.0, 1.0, 1.0));
    // color = pow(color, float3(1.0, 1.0, 1.0) / 2.2);
    return float4(color, 1.0f);
}
